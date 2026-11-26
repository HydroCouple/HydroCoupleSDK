@page sdk_guide_devices 16. Devices

[TOC]

A component that computes on an accelerator keeps its fields in device memory. The HydroCouple
interfaces let such a component hand those fields to others without copying them through the host
first: a `HydroCouple::BufferDescriptor` names the memory space its bytes live in. The HydroCouple
SDK's device module supplies the machinery a component needs on its side of that exchange: a small
backend interface for allocating and copying memory in any space, a host backend that is always
present, a Kokkos backend for accelerators, a registry that owns a component's device mirrors, and
a copy routine that picks the cheapest legal way to move values between two descriptors.

All classes are in the namespace `HydroCouple::SDK::Device` of the core library
`HydroCoupleSDK`. The Kokkos backend is compiled only with `USE_KOKKOS=ON` (@ref sdk_ref_cmake).
The compiled example `docs/examples/ch16_host_backend/` runs on every build; the Kokkos example
`docs/examples/ch16_kokkos_backend/` runs only where Kokkos is installed.

## How exchange items describe device memory

Every value an item hands out or accepts passes through a `HydroCouple::BufferDescriptor`. Four of
its fields say where the bytes are:

| Field | Meaning |
| --- | --- |
| `space` | A `HydroCouple::MemorySpace`: `Host` (ordinary pageable memory), `HostPinned` (page-locked host memory for fast staging), `Device` (accelerator memory) or `Unified` (managed memory reachable from both). |
| `backend` | A `HydroCouple::DeviceBackend` enumerator naming the runtime that owns device memory: `None`, `CUDA`, `HIP`, `SYCL`, `LevelZero`, `OpenCL` or `Other`. |
| `deviceId` | The device ordinal within that runtime. |
| `queue` | An opaque stream or queue handle (a `cudaStream_t`, say) on which an item may enqueue the copy; null means the default stream and a synchronous copy. |

The interfaces set the rules: `getValuesInto()` and `setValuesFrom()` return false with a message
for a memory space the item cannot serve, host-only items accept `MemorySpace::Host` only, and a
component whose items accept device buffers advertises `HydroCouple::Capability::DeviceBuffers`. A
device buffer's pointer must stay valid and fixed between the owning component's `prepare()` and
`finish()`, so kernels can work on it directly. (See `HydroCouple::IComponentDataItem` in the
[HydroCouple interfaces](https://www.hydrocouple.org/HydroCouple/).)

Within the SDK:

- **Every SDK data item is host-only.** The shared hyperslab engine behind the SDK's inputs,
  outputs, arguments and partitioned items refuses any descriptor whose `space` is not `Host` with
  the message "unsupported memory space". The SDK has no data item that serves device memory; the
  device module is for components that build their own.
- **The device module reads `space` and `deviceId` only.** It never reads `backend` or `queue`,
  and every copy it makes is synchronous. The SDK's backend class
  @ref HydroCouple::SDK::Device::DeviceBackend is a different thing from the standard's
  `HydroCouple::DeviceBackend` enumeration, despite the name: the class is an implementation, the
  enumeration a tag.
- **`Unified` and `HostPinned` are host-accessible.** The copy negotiation treats them as host
  memory, and both SDK backends serve them from ordinary host memory.
- **Transports carry host memory only** (@ref sdk_guide_distributed). Stage device values through
  a host buffer before sending them.

A component that holds a field on the device and wants other components to read it serves
`getValuesInto()` by copying from its device mirror into the caller's descriptor with
@ref HydroCouple::SDK::Device::copyBuffer, which handles every legal pairing of spaces, and
serves `setValuesFrom()` the same way in the other direction. It then advertises
`Capability::DeviceBuffers`, which the standard defines as items that can produce and accept
`Device` or `Unified` descriptors, and it sets the `backend` field of the descriptors it creates
itself (the registry leaves it alone, see below).

## Backends

@ref HydroCouple::SDK::Device::DeviceBackend is the seam between the SDK and one accelerator
runtime. It has six operations, and nothing else in the device module touches device memory:

| Method | Does |
| --- | --- |
| `name()` | A human-readable name, used in messages. |
| `deviceCount()` | How many device ordinals the backend addresses. |
| `supportsSpace(space, deviceId)` | Whether `allocate()` and `copy()` accept that space on that ordinal. |
| `allocate(space, deviceId, bytes, message)` | Allocates; null with a message on failure. The caller frees with `deallocate()`. |
| `deallocate(data, space, deviceId)` | Frees; null is a no-op. Never throws. |
| `copy(destination, destinationSpace, destinationDevice, source, sourceSpace, sourceDevice, bytes, message)` | Copies bytes between two supported spaces; complete on return. |
| `synchronize(deviceId, message)` | Waits for outstanding work on the device. |

A backend reports failures through a false or null return and an optional message, never by
throwing, and it tolerates concurrent calls for different allocations; operations on one
allocation are the caller's to order.

### HostBackend

@ref HydroCouple::SDK::Device::HostBackend is always built, so code that negotiates memory spaces
runs on machines without accelerators. Its name is `"host"`, it has no device ordinals, and it
supports `Host`, `HostPinned` and `Unified` on ordinal 0 only, all served by `std::malloc` (pinning
is a device-runtime concept; without one the distinction is a tag). It refuses `Device` and any
other ordinal, refuses zero-byte allocations, and copies with `std::memcpy`, so the ranges must not
overlap. `synchronize()` always succeeds.

### KokkosBackend

@ref HydroCouple::SDK::Device::KokkosBackend implements the seam on Kokkos, built with
`USE_KOKKOS=ON`. The header names no Kokkos type, so a consumer of the installed SDK does not need
Kokkos headers to use it. Whatever execution space Kokkos was configured with (Serial, OpenMP,
CUDA, HIP, SYCL), its default memory space serves `MemorySpace::Device` on ordinal 0, and the
three host spaces are served from `Kokkos::HostSpace`, so under this backend `Unified` is host
memory, not managed memory. The name is `"kokkos-"` followed by the execution space's name, and
`deviceCount()` is 1.

Copies use `Kokkos::deep_copy` between unmanaged byte views and fence afterwards; `synchronize()`
is `Kokkos::fence()`. Kokkos exceptions are caught and returned as messages beginning
"KokkosBackend: ".

The constructor calls `Kokkos::initialize()` when nothing has, and then owns that initialization:
its destructor calls `Kokkos::finalize()`. Kokkos cannot be initialized twice in one process, so a
program keeps one owning backend alive for the whole run (the SDK's test suite holds one in a
global test environment), and frees every Kokkos allocation before that backend is destroyed.
A program that manages Kokkos itself (a `Kokkos::ScopeGuard`) is respected: the backend then
neither initializes nor finalizes.

@snippet ch16_kokkos_backend/main.cpp kokkos

This example builds only with `USE_KOKKOS=ON` (its `REQUIRES` file names that option), so it is
not part of a default test run. The behaviors it relies on (Device allocation, staged and direct
copies) are the ones `tests/hydrocouplesdk/test_device.cpp` checks for every available backend in
one parameterized suite.

## Mirrors: DeviceAllocation and DeviceBufferRegistry

@ref HydroCouple::SDK::Device::DeviceBufferRegistry owns the device copies, or mirrors, of a
component's data items, so their lifetimes are managed in one place. Mirrors are keyed by a string,
normally the item id, and each is described by a
@ref HydroCouple::SDK::Device::DeviceAllocation, which records its data pointer, space, device ordinal, element
kind, shape and size in bytes. The registry borrows its backend, which must outlive it.

| Call | Does | Fails with |
| --- | --- | --- |
| `acquire(key, kind, shape, space, deviceId, message)` | Creates the mirror, or returns the existing one when kind, shape, space and device all match. The contents start uninitialized. | A kind without a fixed width; a negative extent; an existing key with another signature; a space the backend does not support; the backend's allocation failure. |
| `find(key)` | The mirror, or null. | — |
| `upload(key, host, bytes, message)` | Copies host bytes into the mirror. | An unknown key; `bytes` different from the mirror's size; the backend's copy failure. |
| `download(key, host, bytes, message)` | Copies the mirror into host memory. | As for `upload()`. |
| `descriptor(key, out, message)` | Fills a contiguous descriptor over the mirror. | An unknown key. |
| `release(key)` | Frees the mirror; false for an unknown key. | — |
| `count()` | Live mirrors. | — |

The pointer `acquire()` and `find()` return, and the shape a descriptor points to, stay valid until
the key is released or the registry is destroyed, which frees every mirror still held. A mirror of
zero elements reports 0 bytes but holds a one-byte allocation, because backends refuse zero-byte
allocations. `descriptor()` sets `data`, `kind`, `rank`, `shape`, `stridesBytes` (null), `space` and
`deviceId`, and leaves `backend`, `itemSizeBytes` and `queue` as the caller's descriptor held them:
start from a default-constructed descriptor, and set `backend` when a consumer needs the runtime
named.

The registry's bookkeeping is locked, so several threads may acquire and release. An upload or
download runs outside the lock: a concurrent `release()` of the same key is the caller's to
prevent.

## Moving values between spaces

Exchange connections should decide how values move once, at `prepare()`, and then move them every
step without deciding again. @ref HydroCouple::SDK::Device::negotiateCopyPath decides;
@ref HydroCouple::SDK::Device::copyBuffer moves.

<pre class="mermaid">
flowchart TD
    A[source, destination] --> B{same kind, fixed width, same element count?}
    B -- no --> U[Unsupported]
    B -- yes --> C{both host-accessible?}
    C -- yes --> H[HostHyperslab: memcpy, or element-wise for strided views]
    C -- no --> D{backend supports each device side, and each device side contiguous?}
    D -- no --> U
    D -- yes --> E{host side contiguous too?}
    E -- yes --> K[BackendCopy: one backend copy]
    E -- no --> S[StageThroughHost: gather or scatter through a temporary host block]
</pre>

| @ref HydroCouple::SDK::Device::CopyPath | When | What `copyBuffer()` does |
| --- | --- | --- |
| `HostHyperslab` | Both sides `Host`, `HostPinned` or `Unified`. | One `memcpy` when both are contiguous, otherwise an element-wise copy honoring both sides' strides. The backend is not used. |
| `BackendCopy` | A `Device` side is involved and both sides are contiguous. | One `DeviceBackend::copy()` of the whole block. |
| `StageThroughHost` | A `Device` side meets a strided host side. | Gathers the host side into a temporary contiguous block and copies it to the device, or copies the device block to a temporary and scatters it. |
| `Unsupported` | Kinds differ, the kind has no fixed width, counts differ, the backend lacks a device side's space, or a device side is strided. | Nothing; returns false with the reason. |

A descriptor is contiguous when its strides are null or equal to the packed row-major steps
(@ref HydroCouple::SDK::Device::isContiguous; the stride of an extent of 0 or 1 is not checked).
Strided device views are refused rather than copied element by element across the bus. The shapes
of the two sides may differ as long as their element counts agree: elements are taken in row-major
order of the source and placed in row-major order of the destination, so a `{2, N}` block fills a
`{2N}` view. `copyBuffer()` negotiates again on every call; there is no form that takes a path
negotiated earlier, so negotiating at `prepare()` serves to refuse a bad pairing early. Element
sizes come from the kind; `itemSizeBytes` is not read.

## The example

`docs/examples/ch16_host_backend/` exercises the host backend, a registry and the negotiation, and
checks each value. The backend reports what it supports:

@snippet ch16_host_backend/main.cpp backend

A mirror is acquired for a `{2, 3}` field, filled from host values, and the registry's signature
rules refuse a second shape for the same key and a device mirror on a backend without devices:

@snippet ch16_host_backend/main.cpp registry

The negotiation picks the host path for host-accessible pairs and refuses a device side the backend
cannot serve:

@snippet ch16_host_backend/main.cpp negotiate

`copyBuffer()` then fills a flat view of a different shape and a strided view, and the mirror reads
back unchanged:

@snippet ch16_host_backend/main.cpp copy

@snippet ch16_host_backend/main.cpp release

The test `docs.example.ch16_host_backend` prints:

```text
backend "host", 0 device ordinals
  supportsSpace(Host) = true
  supportsSpace(HostPinned) = true
  supportsSpace(Unified) = true
  supportsSpace(Device) = false
mirror 'depth': 48 bytes, rank 2
acquire {6}: DeviceBufferRegistry: 'depth' already registered with a different signature (release() it first)
acquire Device: DeviceBufferRegistry: backend 'host' does not support the requested space/device
mirror -> flat:    HostHyperslab
mirror -> strided: HostHyperslab
flat -> Device:    Unsupported (space transfer: backend 'host' does not support the destination space)
flat:    0.1 0.2 0.3 1.1 1.2 1.3
spread:  0.1 0.0 0.2 0.0 0.3 0.0 1.1 0.0 1.2 0.0 1.3 0.0
live mirrors: 1
after release: 0
```

## Writing a backend

A backend for another runtime derives from @ref HydroCouple::SDK::Device::DeviceBackend and keeps
the runtime's types out of its header, as `KokkosBackend` does, so that the SDK's public headers
never name a vendor type. It must:

- report `supportsSpace()` truthfully, because the registry and the negotiation trust it;
- make `copy()` complete before it returns (the SDK has no asynchronous path), and cover every pair
  of spaces it reports as supported;
- treat a null pointer in `deallocate()` as a no-op and never throw from it;
- return messages rather than throw from every other call.

Run `tests/hydrocouplesdk/test_device.cpp` against it by adding it to the suite's list of backend
cases: the same tests run over every backend.

## Diagnostics

| Message | Cause |
| --- | --- |
| `HostBackend: unsupported space/device` | `Device`, or an ordinal other than 0. |
| `HostBackend: zero-byte allocation` | `bytes` was 0. |
| `HostBackend: allocation of N bytes failed` | `std::malloc` returned null. |
| `HostBackend: copy involves an unsupported space` | A `Device` side, or an ordinal other than 0. |
| `HostBackend: null copy endpoint` | A null source or destination. |
| `KokkosBackend: unsupported space/device`, `KokkosBackend: zero-byte allocation`, `KokkosBackend: allocation of N bytes failed`, `KokkosBackend: copy involves an unsupported space`, `KokkosBackend: null copy endpoint` | As for the host backend, with ordinal 0 the only device. |
| `KokkosBackend: ` followed by the exception text | Kokkos threw during an allocation or copy. |
| ``DeviceBufferRegistry: 'KEY' has no fixed-width element size`` | `acquire()` with String, Opaque or Unknown. |
| ``DeviceBufferRegistry: 'KEY' has a negative extent`` | A negative extent in the shape. |
| ``DeviceBufferRegistry: 'KEY' already registered with a different signature (release() it first)`` | The key exists with another kind, shape, space or device. |
| ``DeviceBufferRegistry: backend 'NAME' does not support the requested space/device`` | The backend's `supportsSpace()` is false. |
| ``DeviceBufferRegistry: unknown key 'KEY'`` | `upload()`, `download()` or `descriptor()` for a key never acquired, or released. |
| ``DeviceBufferRegistry: upload of N bytes into 'KEY' (M bytes)``, ``... download of N bytes from 'KEY' (M bytes)`` | The byte count differs from the mirror's. |
| `space transfer: element kinds differ` | Source and destination kinds differ. |
| `space transfer: kind has no fixed width` | String, Opaque or Unknown. |
| `space transfer: element counts differ` | The two shapes hold different numbers of elements. |
| ``space transfer: backend 'NAME' does not support the source space``, or the same for the destination | A `Device` side the backend cannot serve. |
| `space transfer: strided device source is not supported`, or the same for the destination | A device descriptor with non-packed strides. |
| `unsupported memory space` | An SDK data item was given a non-host descriptor. |

## What to read next

@ref sdk_guide_ports describes the data plane (`getValuesInto()`, `setValuesFrom()`) whose
descriptors this chapter extends to device memory; @ref sdk_guide_distributed explains why
transports carry host memory only.
