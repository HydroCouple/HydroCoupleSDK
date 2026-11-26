@page sdk_guide_distributed 15. Distributed coupling

[TOC]

The distributed layer of the HydroCouple SDK runs parts of a composition outside the process, or
the thread, that owns the workflow. It solves two different problems with one set of pieces:

- **Remote components.** A component runs on another rank (another process under MPI, or another
  thread in tests). The workflow holds a @ref HydroCouple::SDK::Distributed::ProxyModelComponent
  in its place; on the other rank a @ref HydroCouple::SDK::Distributed::ComponentWorker receives
  the proxy's requests and calls the real component.
- **Decomposed domains.** One model's domain is split across ranks, each rank computing its own
  entities and mirroring a few of its neighbors' (ghost or halo entities). A
  @ref HydroCouple::SDK::Distributed::PartitionedDataItem holds one rank's values and refreshes
  the mirrors through a @ref HydroCouple::SDK::Distributed::HaloExchanger.

Both sit on one seam: the standard's transport interface, `HydroCouple::Distributed::ITransport`
(declared in `hydrocoupledistributed.h` of the [HydroCouple interfaces](https://www.hydrocouple.org/HydroCouple/)).
The SDK provides two transports behind it: an in-process hub for testing and single-process runs,
and an MPI transport. Everything above the seam is the same code for both. This chapter describes
the transports, the wire format they carry, the proxy and worker protocol, partitioned items and
halo exchange, and the flux-and-derivative payload convention, with two compiled examples that
run on the in-process transport (`docs/examples/ch15_proxy_worker/` and
`docs/examples/ch15_halo_exchange/`) and the MPI example the SDK's CI runs
(`examples/distributed_coupling/`).

All classes are in the namespace `HydroCouple::SDK::Distributed` and in the core library
`HydroCoupleSDK`. The MPI transport is compiled only when the SDK is configured with
`USE_MPI=ON` (@ref sdk_ref_cmake); everything else is always built.

## The pieces

| Class | Role | Implements |
| --- | --- | --- |
| @ref HydroCouple::SDK::Distributed::InProcHub | A set of rank-addressed mailboxes inside one process. | — |
| @ref HydroCouple::SDK::Distributed::InProcTransport | One rank's endpoint on a hub. | `ITransport` |
| @ref HydroCouple::SDK::Distributed::MpiTransport | One MPI process's endpoint over a duplicate of `MPI_COMM_WORLD`. | `ITransport` |
| @ref HydroCouple::SDK::Distributed::MpiEnvironment | Initializes and finalizes MPI for programs that own it. | — |
| @ref HydroCouple::SDK::Distributed::Envelope, @ref HydroCouple::SDK::Distributed::OpCode | The 32-byte header of every proxy and worker message. | — |
| @ref HydroCouple::SDK::Distributed::ValuePacker | The typed byte block every payload travels as. | — |
| @ref HydroCouple::SDK::Distributed::ProxyModelComponent | Local stand-in for a remote component. | `IProxyModelComponent` |
| @ref HydroCouple::SDK::Distributed::ComponentWorker | Remote-side loop that serves one component. | — |
| @ref HydroCouple::SDK::Distributed::PartitionedDataItem | One rank's owned and mirrored values of a decomposed field. | `IPartitionedComponentDataItem` |
| @ref HydroCouple::SDK::Distributed::HaloExchanger | Builds the exchange pattern and runs each exchange. | — |
| @ref HydroCouple::SDK::Distributed::FluxDerivativePayload | A flux and its derivative as one `{2, N}` block. | — |

<pre class="mermaid">
flowchart LR
    subgraph rank0 [Rank 0: the host]
        WF[Workflow] --> PX[ProxyModelComponent]
        P0[PartitionedDataItem] --- H0[HaloExchanger]
    end
    subgraph rank1 [Rank 1]
        WK[ComponentWorker] --> MC[Model component]
        P1[PartitionedDataItem] --- H1[HaloExchanger]
    end
    PX -- "requests and replies (Envelope + ValuePacker blocks)" --- T[(ITransport: InProcTransport or MpiTransport)]
    WK --- T
    H0 -- "halo blocks" --- T
    H1 --- T
</pre>

## Transports

A transport moves typed buffers between ranks. A payload is always described by a
`HydroCouple::BufferDescriptor` (data pointer, element kind, rank, shape, optional byte strides,
memory space), never by raw bytes, and it arrives in a buffer the receiver has already allocated.
An endpoint is a `HydroCouple::Distributed::ITransport::Endpoint`; both SDK transports address
peers by `rank` alone and ignore `address`.

Messages are matched on (source rank, tag). Between one sender, one receiver and one tag, messages
arrive in the order they were sent; messages on different tags may be received in any order, which
is how the protocol keeps concurrent streams apart. A negative source rank (by convention `-1`)
receives from any sender.

### What the two transports do

| Behavior | `InProcTransport` | `MpiTransport` |
| --- | --- | --- |
| Participants | The hub's rank count, fixed at `InProcHub::create(n)` (at least 1). | The size of `MPI_COMM_WORLD`. |
| Endpoint address | `"inproc"` | `"mpi"` |
| `send()` | Packs the payload into the receiver's mailbox and returns; never blocks. | Packs, then `MPI_Send`; may wait for the receiver on large messages. |
| `receive()` | Blocks until a match arrives; no timeout. | `MPI_Mprobe` then `MPI_Mrecv`; blocks, no timeout. |
| `sendAsync()` | Sends at once and returns a completed request. | Packs and posts `MPI_Isend`; the request owns the packed bytes, so the caller's buffer is free on return. |
| `receiveAsync()` | Matches lazily, when `test()`, `wait()` or `waitFor()` runs. | Matched probe (`MPI_Improbe`/`MPI_Imrecv`) when `test()` or `wait()` first finds the message. |
| `waitFor(seconds)` | Waits on a condition variable until the deadline. | Polls `test()` every 50 µs until the deadline. |
| `cancel()` | Does not cancel; returns `test()`. | Does not cancel; returns `test()`. |
| Destroying a pending request | Does not wait; an unmatched message stays queued. | A send waits for completion; a receive waits only if it had started. |
| Lost peer | Cannot happen in one process. | Not detected; under MPI's default error handler a lost rank aborts the job. |

Both transports pack every payload with ValuePacker, so a receive checks that the arriving block
has the kind and the element count of the destination: a `DataKind::Int32` payload refuses to land
in a `Float64` buffer ("kind mismatch"), and a payload of 6 elements refuses a destination of 4
("buffer element count mismatch"). The shapes need not match, only the counts. Strided sources are
gathered on send and strided destinations scattered on receive. Only host memory and fixed-width
kinds travel: `DataKind::String`, `Opaque` and `Unknown` are refused, and so is any buffer whose
`space` is not `MemorySpace::Host`.

### The in-process hub

@ref HydroCouple::SDK::Distributed::InProcHub::create makes a hub for a fixed number of ranks, and
@ref HydroCouple::SDK::Distributed::InProcHub::createTransport makes the transport for one rank
(nullptr for a rank outside `[0, n)`). The hub is held by `std::shared_ptr`; each transport and
each pending receive keeps it alive. Every rank's mailbox has its own lock, so all transport calls
may be made from any thread. Create exactly one transport per rank: two transports for one rank
would read the same mailbox.

Use it to test distributed code without a launcher (the SDK's own suites `test_distributed` and
`test_fluxpayload` run the whole protocol over it), and to run ranks as threads of one process.

### The MPI transport

@ref HydroCouple::SDK::Distributed::MpiTransport duplicates `MPI_COMM_WORLD` when it is
constructed, so its tags cannot collide with the application's own MPI traffic. The constructor is
collective (every rank must construct its transport), MPI must already be initialized, and the
transport does not finalize MPI. @ref HydroCouple::SDK::Distributed::MpiTransport::available
tells whether MPI is initialized and not yet finalized.

A program that owns MPI creates a @ref HydroCouple::SDK::Distributed::MpiEnvironment first. It
calls `MPI_Init_thread` requesting `MPI_THREAD_SERIALIZED` when MPI is not initialized, and
`MPI_Finalize` on destruction only if it initialized MPI.
@ref HydroCouple::SDK::Distributed::MpiEnvironment::providedThreadLevel returns the level MPI
granted and @ref HydroCouple::SDK::Distributed::MpiEnvironment::threadLevelSufficient checks it
against the SDK's minimum, `MPI_THREAD_FUNNELED` (nothing in the distributed layer starts a
thread). A host that calls one transport from several threads at once needs
`MPI_THREAD_MULTIPLE` and must check the provided level itself.

The MPI transport cannot detect a lost peer: a pending operation with a rank that has died is
never completed as failed. Callers that must not hang bound their waits with the asynchronous
calls and `waitFor()`.

## The wire format

### Payload blocks

@ref HydroCouple::SDK::Distributed::ValuePacker turns a typed buffer into one contiguous byte
block and back. The block is a fixed header, the shape, and the elements in row-major order, all
native-endian:

| Bytes | Content |
| --- | --- |
| 4 | element kind, the integer value of `HydroCouple::DataKind` |
| 4 | rank r (number of dimensions) |
| 8 r | the extents, one `int64_t` per dimension |
| count × size | the elements, C-contiguous |

A `{2, 3}` Float64 block is therefore 8 + 16 + 48 = 72 bytes. The element size comes from the kind
(1 byte for Boolean and the 8-bit integers, 2, 4 or 8 for the others); `itemSizeBytes` is not read.
`packedSize()` returns 0 for a kind without a fixed width. `peek()` reads the kind and shape of a
block without unpacking it, for a receiver that must size its buffer first. Both transports use the
same block, which is why they are interchangeable: a receiver never needs to know which one carried
the message. Byte order is not converted, so ranks must share an endianness.

### Envelopes and op codes

The proxy and worker exchange @ref HydroCouple::SDK::Distributed::Envelope headers: 32 bytes of
plain data, sent as a rank-1 `UInt8` payload. Build one with
@ref HydroCouple::SDK::Distributed::makeEnvelope, which stamps the magic number `0x4843504C`
("HCPL") and protocol version 2 that `valid()` checks.

| Field | Type | Meaning |
| --- | --- | --- |
| `magic` | `uint32_t` | `Envelope::kMagic`; anything else is not a protocol message. |
| `version` | `uint8_t` | `Envelope::kVersion` (2). |
| `opCode` | `uint8_t` | The @ref HydroCouple::SDK::Distributed::OpCode; read it with `op()`. |
| `flags` | `uint16_t` | Bit 15 (`kFlagStatusPresent`) set: the low byte is the replier's `ComponentStatus`. |
| `sourceRank` | `int32_t` | Sender's rank; the worker replies to it. |
| `targetRank` | `int32_t` | Receiver's rank (written, not read). |
| `tag` | `uint32_t` | The request's tag: its payload and its replies travel on it. |
| `epoch` | `uint32_t` | A synchronization stamp (written, not read by the SDK). |
| `payloadBytes` | `uint64_t` | Size of the payload message that follows on `tag`; 0 for none. |

| Op code | Sent by | Payload | Worker's answer |
| --- | --- | --- | --- |
| `Hello` (0) | `connectToPeer()` | — | Ack |
| `Initialize` (2) | `initialize()` | — | Ack if the component reaches `Initialized`, else Error |
| `Validate` (3) | `validate()` | — | Ack if `validate()` returns no problems, else Error with them joined by "; " |
| `Prepare` (4) | `prepare()` | — | Ack unless the component is then `Failed` |
| `Update` (5) | `update()` | — | Ack unless the component is then `Failed` |
| `Finish` (6) | `finish()` | — | Ack |
| `GetValues` (7) | `getRemoteValues()` | item id (UTF-8) | Ack followed by the item's whole value block, or Error |
| `SetValues` (8) | `setRemoteValues()` | item id, then a second message with the value block | Ack or Error |
| `Status` (9) | nothing in the SDK | — | Error ("unsupported op code 9") |
| `Heartbeat` (10) | `ping()` | — | Ack |
| `Shutdown` (12) | the host, by hand | — | Ack, then the worker's loop ends |
| `Ack` (1), `Error` (11) | the worker | Error carries its text | — |

## Remote components: the proxy and the worker

<pre class="mermaid">
sequenceDiagram
    participant H as Host (rank 0)
    participant P as ProxyModelComponent
    participant T as Transport
    participant W as ComponentWorker (rank 1)
    participant C as Component
    H->>P: update()
    P->>T: Envelope(Update, tag n) on tag 0
    T->>W: receive on tag 0 from any rank
    W->>C: update()
    C-->>W: status Done
    W->>T: Envelope(Ack with status Done) on tag n
    T-->>P: reply on tag n (polled every 200 µs until the timeout)
    P-->>H: status Done (mirrored)
</pre>

Every request is a round trip. The proxy takes a fresh tag (counting up from 1), sends the
envelope on tag 0, which is the control tag the worker listens on, and sends any payload on the
fresh tag. It then waits for the reply envelope on the fresh tag, polling every 200 µs until
@ref HydroCouple::SDK::Distributed::ProxyModelComponent::requestTimeout elapses (30 s unless set).
An Ack carries the component's status after the request in its flags, and the proxy records it;
an Error is followed by its text. A reply that arrives after its request timed out is never read.

The worker is stateless between messages. Each
@ref HydroCouple::SDK::Distributed::ComponentWorker::serveOne receives one envelope from any rank
on tag 0, receives the announced payload on the envelope's tag, calls the component and replies to
the envelope's source on the same tag.
@ref HydroCouple::SDK::Distributed::ComponentWorker::run loops until a Shutdown request and returns
the number of messages served. An envelope that fails to arrive or is not `valid()` is dropped
without a reply. Item ids are looked up among the component's outputs, then its inputs.

### The example

`docs/examples/ch15_proxy_worker/` runs a three-reach storage component on rank 1 of a two-rank
hub and drives it from rank 0. The remote component is ordinary SDK code; nothing in it knows it is
remote. It advances each reach storage \f$S\f$ (m³) by
\f$S \leftarrow S + \Delta t\,(I - kS)\f$ and publishes the outflow \f$Q = kS\f$, with
\f$\Delta t = 60\f$ s and \f$k = 0.01\f$ s⁻¹, and reports `Done` after four steps. As in every SDK
component, the items are `std::unique_ptr` members declared after the metadata they point to;
`addInput()` and `addOutput()` only register them.

@snippet ch15_proxy_worker/main.cpp remote

The host side creates the hub and one transport per rank, starts the worker on its own thread, and
creates the proxy with the worker's rank:

@snippet ch15_proxy_worker/main.cpp ranks

The lifecycle calls on the proxy become requests. `connectToPeer()` and `ping()` check that the
worker answers; they are optional (no lifecycle call requires a connection):

@snippet ch15_proxy_worker/main.cpp lifecycle

The proxy has no exchange items of its own, so a workflow cannot connect ports to it; values move
through @ref HydroCouple::SDK::Distributed::ProxyModelComponent::setRemoteValues and
@ref HydroCouple::SDK::Distributed::ProxyModelComponent::getRemoteValues, each a whole-item
transfer whose destination or source must have the item's kind and element count. The loop pushes
an inflow, steps the remote component, pulls the outflow and compares it with the same computation
done locally:

@snippet ch15_proxy_worker/main.cpp values

The proxy mirrors the peer's status after every update: it reports `Done` when the peer reaches
`Done`, so a time-stepped workflow waiting for every component to finish stops. Errors from the
peer come back as messages, and the peer stays alive:

@snippet ch15_proxy_worker/main.cpp errors

The proxy has no call that stops the worker. The host sends the Shutdown request itself:

@snippet ch15_proxy_worker/main.cpp shutdown

Finally a proxy whose worker never answers shows the failure path: every request times out, and
the lifecycle call latches `Failed`.

@snippet ch15_proxy_worker/main.cpp deadpeer

The test `docs.example.ch15_proxy_worker` prints:

```text
after prepare: proxy Updated, remote Updated
step 1  proxy Updated  Q = 0.700000 1.400000 2.400000
step 2  proxy Updated  Q = 0.730000 1.460000 2.760000
step 3  proxy Updated  Q = 0.967000 1.934000 3.804000
step 4  proxy Done     Q = 1.399300 2.798600 5.571600
getRemoteValues("stage"): false, "remote component has no data item 'stage'"
after finish: proxy Finished, remote Finished
worker served 20 messages
orphan: Failed, 1 error(s)
  fatal: remote initialize failed: request timed out after 0.200000 s (op 2)
```

Twenty messages: Hello, Heartbeat, Initialize, Validate and Prepare; a SetValues, an Update and a
GetValues for each of the four steps; the refused GetValues; Finish; and Shutdown.

### What each proxy call does to its status

| Call | While waiting | On Ack | On an Error reply | On a timeout or transport failure |
| --- | --- | --- | --- | --- |
| `initialize()` | `Initializing` | `Initialized` | `Failed` | `Failed` |
| `validate()` | `Validating` | `Valid`, returns `{}` | `Invalid`, a `Severity::Error` entry, returns the problems | `Failed`, returns the failure |
| `prepare()` | `Preparing` | `Updated` | `Failed` | `Failed` |
| `update()` | `Updating` | `Done`, `WaitingForData` or `Updated`, as the peer reports | `Failed` | `Failed` |
| `finish()` | `Finishing` | `Created` if the peer reports `Created`, else `Finished` | `Failed` | `Failed` |

`Failed` comes with a `Severity::Fatal` entry on the proxy's error queue (the message begins
"remote initialize failed: ", "remote prepare failed: " and so on) and clears the connection flag.
A peer that sends no status flag (one older than `kFlagStatusPresent`) is taken to be `Updated`
after an update and `Finished` after a finish. `getRemoteValues()`, `setRemoteValues()` and
`ping()` never change the status.

### Settings and their limits

- **Request timeout.** `setRequestTimeout(seconds)` takes effect for every later request. A
  non-positive value is ignored and the current timeout kept: the proxy has no wait-forever mode,
  although `IProxyModelComponent` allows one. `ping(timeoutSeconds)` uses its own timeout when it
  is positive, otherwise the request timeout.
- **Identity.** `remoteId()` returns the id given to the constructor; it is not checked against
  the peer. `remoteEndpoint()` is the local endpoint's address with the worker's rank.
- **Partition queries.** `partitionCount()` returns the transport's participant count and
  `partitionRank()` the proxy's own rank. They describe the transport, not a decomposition of the
  remote component's domain.
- **Threads.** Drive a proxy from one thread, as any component. The worker's `run()` blocks its
  thread; the component it serves must not be touched by anyone else meanwhile.

### Running components in other processes

The SDK provides the two ends and the transport; it does not provide a launcher, and no SDK code
creates the remote component for you. Under MPI a host program follows the same shape as the
example, with ranks as processes instead of threads:

1. Every process creates an `MpiEnvironment` and then an `MpiTransport` (collective).
2. Rank 0 builds the workflow with a `ProxyModelComponent` for each remote component, constructed
   with the rank that serves it.
3. Every other rank constructs its own component (from a component info or a plugin library, as
   it would locally), wraps it in a `ComponentWorker` and calls `run()`.
4. When the workflow has finished, rank 0 sends each worker a Shutdown envelope on tag 0, and every
   rank leaves its scope so the transport and then the environment are destroyed.

Only the transport changes between the in-process example and this arrangement. The SDK's MPI CI
job proves the transport itself (`MpiTransportSuite`, the `test_mpitransport` tests under three
ranks) and runs `examples/distributed_coupling` on four ranks; the proxy and worker are tested over
the in-process transport only.

## Decomposed domains: partitioned items and halo exchange

A domain decomposition splits a field's entities (cells, nodes, reaches) among ranks. Each rank
*owns* some entities, which it computes, and *mirrors* a few that other ranks own, because its own
computation reads them. The mirrors are called virtual, ghost or halo entities. The SDK's policy,
from the SWMM engine's experience: virtual entities carry no degrees of freedom. Solvers write
owned values only, and every exchange rebuilds the mirrors from their owners, so a decomposed
quantity is never counted twice.

### PartitionedDataItem

`PartitionedDataItem<T>` (@ref HydroCouple::SDK::Distributed::PartitionedDataItem) stores one
rank's values in one array: positions `[0, owned)` hold the owned entities in the order of their global indexes as
given, and positions `[owned, owned + virtual)` hold the mirrors in the order given. Its data plane
(`shape()`, `getValuesInto()`, `setValuesFrom()`) spans the whole local array, and its
`IPartitionedComponentDataItem` queries expose the decomposition:

| Member | Returns |
| --- | --- |
| `partitionedDimension()` | 0: the item has one (entity) dimension. |
| `globalCount()` | the global entity count passed to the constructor (not checked). |
| `ownedGlobalIndexes()` | the global index of each owned position. |
| `virtualGlobalIndexes()`, `virtualOwners()` | the global index and owner rank of each mirror. |
| `synchronizationEpoch()` | how many exchanges have completed (0 at first). |
| `synchronizeAsync()` | starts an exchange; the returned request completes it. |
| `ownedValues()`, `virtualValues()` | typed views of the two regions. |
| `exchanger()` | the embedded HaloExchanger. |

The library instantiates `double` and `int32_t` (`PartitionedDataItemDouble`,
`PartitionedDataItemInt`). The item has no dimensions and no value definition: it is a building
block for a component's decomposed field, not a port a composition connects. The read-only policy
for mirrors is a convention; `setValuesFrom()` can write any position, and the next exchange
overwrites the mirrors.

### HaloExchanger

@ref HydroCouple::SDK::Distributed::HaloExchanger runs the communication for one item, over any
transport. Use it directly when a component keeps its own arrays in the owned-then-virtual layout.

**Pattern.** `buildPattern()` is collective: every rank of the transport calls it at the same
time, typically from `prepare()`. Each rank sends every other rank the number of globals it needs
from it (zero included) and then their list, and receives the same from every peer, at most
\f$2(P-1)\f$ messages per rank for \f$P\f$ ranks. The owner checks each requested global against
the globals it owns: a request for one it does not own fails that rank's `buildPattern()` with
"rank P requested global G, which rank R does not own". A virtual entity that lists its own rank
as owner, or owner and index lists of different lengths, fail before anything is sent.
`PartitionedDataItem::synchronizeAsync()` builds the pattern itself the first time when nobody
did.

**Epoch.** Each exchange has three steps:

1. `begin()` copies the owned values each peer asked for into per-peer staging, sends them, and
   posts a nonblocking receive per owner peer. The values that travel are those at this moment.
2. The caller computes on owned entities, the overlap window. Owned values may change now without
   affecting the exchange.
3. `end()` waits for every receive, copies each halo into its virtual slots and advances the epoch;
   `tryEnd()` does the same without blocking and reports whether it finished.

`synchronizeAsync()` is `begin()`, and the request it returns runs `end()` on `wait()` and
`tryEnd()` on `test()`. A second `begin()` before the first epoch ended fails with "previous epoch
not ended"; a failed receive ends the epoch without advancing it.

**Tags.** Pattern negotiation uses `baseTag` and `baseTag + 1`; epoch \f$e\f$ sends its halos on
\f$\text{baseTag} + 2 + (e \bmod 1024)\f$. An item therefore occupies tags `baseTag` to
`baseTag + 1025` (the default `baseTag` is 2000), and two items on one transport need base tags at
least 1026 apart. The proxy's request tags count up from 1 on whatever transport it uses, so keep
partitioned items' tags above any proxy traffic they share a transport with.

**Blocking.** Sends are posted before receives with the transport's blocking `send()`. The
in-process `send()` never blocks; `MPI_Send` may hold a large halo until the neighbor receives it,
depending on the MPI library's eager-message limit.

### The example

`docs/examples/ch15_halo_exchange/` splits a 12-cell diffusion chain over three ranks, four cells
each. Each rank mirrors the cell on each side that a neighbor owns, steps the explicit diffusion
update \f$u_i \leftarrow u_i + \alpha (u_{i-1} - 2u_i + u_{i+1})\f$ with \f$\alpha = 0.25\f$ five
times, and the result is compared bit for bit with the same computation on one rank:

@snippet ch15_halo_exchange/main.cpp rank

Each rank is a thread with its own transport on one hub:

@snippet ch15_halo_exchange/main.cpp threads

Because each rank's halo carries exactly the values its neighbor holds, the decomposed run performs
the same floating-point operations as the serial one and agrees to the last bit:

```text
rank 0 owns [0, 4), mirrors 1, sends to 1 peer(s), epochs 5
rank 1 owns [4, 8), mirrors 2, sends to 2 peer(s), epochs 5
rank 2 owns [8, 12), mirrors 1, sends to 1 peer(s), epochs 5
cell  serial      distributed
   0  1.000000000 1.000000000
   1  0.999023438 0.999023438
   2  0.989257812 0.989257812
   3  0.945312500 0.945312500
   4  0.828125000 0.828125000
   5  0.623046875 0.623046875
   6  0.376953125 0.376953125
   7  0.171875000 0.171875000
   8  0.054687500 0.054687500
   9  0.010742188 0.010742188
  10  0.000976562 0.000976562
  11  0.000000000 0.000000000
owner's buildPattern: HaloExchanger: rank 0 requested global 3, which rank 1 does not own
linearized flux at h = 2.10 m: 1.1600 m3/s
receiving 2 entities into 3: ValuePacker::unpack: buffer element count mismatch
sizeof(Envelope) = 32, packed {2, 3} Float64 block = 72 bytes
```

A misattributed owner is caught by the owner while the pattern is built:

@snippet ch15_halo_exchange/main.cpp misowned

### The same code under MPI

`examples/distributed_coupling/main.cpp` decomposes a 24-reach chain over however many MPI ranks
it is launched with, and the SDK's CI runs it with `mpiexec -n 4`. It is the in-process example's
structure with `MpiEnvironment` and `MpiTransport` in place of the hub. The setup:

@dontinclude distributed_coupling/main.cpp
@skip MpiEnvironment environment
@until const int32_t size
@skip PartitionedDataItem<double> flow
@until 900);

and the start of the time loop, with the overlap window between `synchronizeAsync()` and
`wait()`:

@skip for (int step = 0
@until exchange->wait();

The MPI test suite (`tests/hydrocouplesdk/test_mpitransport.cpp`, run as `MpiTransportSuite`
under three ranks) runs the same ring exchange as the in-process suite, plus the transport's own
tests: typed round trips, any-source and out-of-order tags, asynchronous sends with matched-probe
receives, strided sources and kind mismatches.

## Flux and derivative payloads

Exchanging a bare flux between loosely coupled components is a known source of instability: the
receiver applies a flux computed from the sender's old state. Exchanging the flux together with
its derivative with respect to the coupling state lets the receiver linearize the flux inside its
own iteration:

\f[ q(h) \approx q_0 + \frac{\partial q}{\partial h}\,(h - h_0) \f]

where \f$q_0\f$ is the flux computed at state \f$h_0\f$ and \f$h\f$ is the receiver's current
state. @ref HydroCouple::SDK::Distributed::FluxDerivativePayload holds N such pairs as one Float64
block of shape `{2, N}`: row 0 the fluxes, row 1 the derivatives. It adds no wire format; the block
travels through any transport like any other payload. `flux()` and `derivative()` are views of the
two rows, `descriptor()` is the whole block (valid while the payload lives, and usable as a receive
destination), `linearizedFlux(index, state, reference)` evaluates the formula (and throws
`std::out_of_range` for a bad index), and `conformsToConvention(descriptor, N, message)` checks a
descriptor received from a peer: Float64, rank 2, first extent 2, second extent N (any N when N is
negative). Units are the caller's; for a discharge in m³/s and a head in meters the derivative is
in m²/s.

@snippet ch15_halo_exchange/main.cpp flux

The transport's own checks already refuse a block of the wrong entity count, as the example's last
lines show. The wire sizes:

@snippet ch15_halo_exchange/main.cpp wire

## Diagnostics

| Message | Cause | Fix |
| --- | --- | --- |
| `InProcTransport::send: invalid target rank N` | `to.rank` outside `[0, participantCount())`. | Use a rank the hub was created with. |
| `InProcTransport::receive: invalid source rank N` | `from.rank` at or above the participant count. | Use a valid rank, or -1 for any. |
| `MpiTransport::send: invalid target rank N`, `MpiTransport::sendAsync: invalid target rank N` | As above, for the MPI size. | |
| `MpiTransport::receive: invalid source rank N`, `MpiTransport::receiveAsync: invalid source rank N` | As above. | |
| `MpiTransport::send: ...` (and `receive`, `sendAsync`, `receiveAsync`) followed by the MPI error text | An MPI call returned an error. | See the MPI library's documentation. |
| `ValuePacker::pack: source has a non-fixed-width DataKind` | A String, Opaque or Unknown payload. | Send text as UInt8 bytes. |
| `ValuePacker::pack: source must be host-resident`, `ValuePacker::unpack: destination must be host-resident` | A device buffer was given to a transport. | Stage through host memory (@ref sdk_guide_devices). |
| `ValuePacker::unpack: kind mismatch` | The destination's kind is not the sent kind. | Receive into the sender's kind. |
| `ValuePacker::unpack: buffer element count mismatch` | The destination holds a different number of elements. | Size the destination from the sender's shape (`ValuePacker::peek`). |
| `ValuePacker::unpack: block size N does not match its header (M)`, `ValuePacker::peek: ...` | A truncated or corrupted block. | |
| `request timed out after S s (op N)` | The worker did not answer within the request timeout. | Check that the worker runs on the proxy's `remoteRank`; raise the timeout for slow steps. |
| `remote initialize failed: ...` (and prepare, update, finish, validate) | A lifecycle round trip failed; the proxy is `Failed`. | Read the rest of the message: a timeout or the worker's error. |
| `remote initialize() did not reach Initialized`, `remote prepare() failed`, `remote update() failed` | The real component did not reach the expected status. | Read the remote component's own errors on its rank. |
| `remote component has no data item 'ID'` | `getRemoteValues`/`setRemoteValues` named an item the component lacks. | Use an input or output id of the remote component. |
| `item 'ID' has a non-fixed-width DataKind` | GetValues on a String or Opaque item. | |
| `SetValues value block rejected: ...` | The pushed block's kind or count differs from the item's. | Send the item's kind and element count. |
| `worker failed to receive the request payload: ...` | The payload announced by an envelope did not arrive intact. | |
| `unsupported op code N` | An op code the worker does not serve (`Status`, or an unknown byte). | |
| `malformed reply envelope`, `remote error` | A reply failed `valid()`, or an Error reply carried no text. | |
| `HaloExchanger: virtualOwners must parallel virtualGlobalIndexes` | The owner list and the mirror list differ in length. | |
| `HaloExchanger: virtual global G lists this rank as its own owner` | A mirror names its own rank as owner. | Own it instead, or fix the owner. |
| `HaloExchanger: rank P requested global G, which rank R does not own` | Inconsistent ownership between ranks. | Make the owner lists agree. |
| `HaloExchanger::begin: buildPattern() has not run`, `... previous epoch not ended` | Out-of-order calls. | Build the pattern; complete each request before the next exchange. |
| `HaloExchanger::end: no active epoch`, `HaloExchanger::tryEnd: no active epoch` | `end()` without `begin()`. | |
| `HaloExchanger: halo from rank P failed: ...` | A halo receive failed (kind or size). | |
| `flux+derivative payload must be Float64` (and rank, row-dimension and entity-count messages) | A received descriptor breaks the `{2, N}` convention. | |

## What to read next

The API reference of each class gives every parameter and failure. @ref sdk_guide_devices covers
device memory, which transports do not carry; @ref sdk_guide_workflow covers the workflows a proxy
takes part in; @ref sdk_ref_cmake lists `USE_MPI` and the MPI test options.
