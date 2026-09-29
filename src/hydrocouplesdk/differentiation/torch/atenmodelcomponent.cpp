// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// AtenModelComponent -- see atenmodelcomponent.h.

#include "hydrocouplesdk/differentiation/torch/atenmodelcomponent.h"
#include "hydrocouplesdk/differentiation/differentials.h"

#include <cstdio>
#include <cstdlib>
#include <exception>

namespace HydroCouple::SDK::Differentiation::Torch {

using HydroCouple::BufferDescriptor;
using HydroCouple::DataKind;
using HydroCouple::DifferentialEntry;
using HydroCouple::DifferentialRole;
using HydroCouple::DifferentialSet;
using HydroCouple::IComponentDataItem;
using HydroCouple::MemorySpace;
using CStatus = HydroCouple::IModelComponent::ComponentStatus;

namespace {

bool fail(std::string *message, const std::string &text)
{
    if (message)
        *message = text;
    return false;
}

BufferDescriptor describe(torch::Tensor &t, const std::vector<int64_t> &shape)
{
    BufferDescriptor d;
    d.data = t.data_ptr<double>();
    d.kind = DataKind::Float64;
    d.rank = static_cast<int32_t>(shape.size());
    d.shape = shape.empty() ? nullptr : shape.data();
    d.stridesBytes = nullptr;
    d.space = MemorySpace::Host;
    return d;
}

//! A differential buffer as a new tensor of \p shape.
bool readEntry(const DifferentialEntry &entry, const std::vector<int64_t> &shape,
               torch::Tensor &out, std::string *message)
{
    std::vector<double> values;
    if (!readDifferential(entry, shape, values, message))
        return false;
    out = torch::from_blob(values.data(), shape, torch::kFloat64).clone();
    return true;
}

//! Overwrites a differential buffer with \p value (undefined: zeros).
bool writeEntry(const DifferentialEntry &entry, const std::vector<int64_t> &shape,
                const torch::Tensor &value, std::string *message)
{
    std::vector<double> values(static_cast<size_t>(elementCount(shape)), 0.0);
    if (value.defined())
    {
        torch::Tensor c = value.detach().to(torch::kCPU, torch::kFloat64).contiguous();
        if (c.numel() != static_cast<int64_t>(values.size()))
            return fail(message, "derivative has the wrong number of elements");
        std::copy(c.data_ptr<double>(), c.data_ptr<double>() + c.numel(), values.begin());
    }
    return writeDifferential(entry, shape, values, message);
}

template <typename T>
int indexOf(const std::vector<T *> &items, const IComponentDataItem *item)
{
    for (size_t i = 0; i < items.size(); ++i)
        if (static_cast<const IComponentDataItem *>(items[i]) == item)
            return static_cast<int>(i);
    return -1;
}

std::vector<int64_t> shapeOf(const torch::Tensor &t)
{
    return t.sizes().vec();
}

std::string shapeString(const std::vector<int64_t> &shape)
{
    std::string s = "[";
    for (size_t i = 0; i < shape.size(); ++i)
        s += (i ? ", " : "") + std::to_string(shape[i]);
    return s + "]";
}

} // namespace

// ============================================================================
// Items <-> tensors
// ============================================================================

torch::Tensor readItem(const IComponentDataItem *item, std::string *message)
{
    if (!item)
    {
        fail(message, "null data item");
        return {};
    }
    if (item->dataKind() != DataKind::Float64)
    {
        fail(message, "data item \"" + item->id() + "\" is not Float64");
        return {};
    }
    const std::vector<int64_t> shape = item->shape();
    torch::Tensor t = torch::empty(shape, torch::kFloat64);
    const std::vector<int64_t> start(shape.size(), 0);
    if (!item->getValuesInto(describe(t, shape), start, shape, message))
        return {};
    return t;
}

bool writeItem(IComponentDataItem *item, const torch::Tensor &values, std::string *message)
{
    if (!item)
        return fail(message, "null data item");
    if (item->dataKind() != DataKind::Float64)
        return fail(message, "data item \"" + item->id() + "\" is not Float64");
    const std::vector<int64_t> shape = item->shape();
    torch::Tensor c = values.detach().to(torch::kCPU, torch::kFloat64).contiguous();
    if (shapeOf(c) != shape)
        return fail(message, "a tensor of shape " + shapeString(shapeOf(c)) +
                                 " cannot be written to data item \"" + item->id() +
                                 "\" of shape " + shapeString(shape));
    const std::vector<int64_t> start(shape.size(), 0);
    return item->setValuesFrom(describe(c, shape), start, shape, message);
}

// ============================================================================
// Lifecycle
// ============================================================================

AtenModelComponent::AtenModelComponent(std::string_view id)
    : AbstractModelComponent(id)
{}

std::vector<std::string> AtenModelComponent::validate()
{
    setStatus(CStatus::Validating);
    setStatus(CStatus::Valid);
    return {};
}

void AtenModelComponent::prepare()
{
    setStatus(CStatus::Preparing);
    m_steps = 0;
    m_stepIndex = 0;
    m_haveStep = false;
    setStatus(CStatus::Updated);
}

void AtenModelComponent::update(const std::vector<HydroCouple::IOutput *> &)
{
    setStatus(CStatus::Updating);
    applyInputValues();

    std::string message;
    Point point;
    point.step = m_steps + 1;
    if (!gather(point, message))
    {
        setStatus(CStatus::Failed, message);
        return;
    }

    StepResult result;
    try
    {
        torch::NoGradGuard noGrad;
        m_stepIndex = point.step;
        result = step(point.states, point.inputs, point.arguments);
    }
    catch (const std::exception &e)
    {
        setStatus(CStatus::Failed, std::string("step() threw: ") + e.what());
        return;
    }
    if (!checkResult(result, message))
    {
        setStatus(CStatus::Failed, message);
        return;
    }

    const auto states = differentiableStates();
    const auto outputs = differentiableOutputs();
    for (size_t i = 0; i < states.size(); ++i)
        if (!writeItem(states[i], result.states[i], &message))
        {
            setStatus(CStatus::Failed, message);
            return;
        }
    for (size_t i = 0; i < outputs.size(); ++i)
        if (!writeItem(outputs[i], result.outputs[i], &message))
        {
            setStatus(CStatus::Failed, message);
            return;
        }

    m_point = std::move(point);
    m_haveStep = true;
    ++m_steps;
    afterStep(result);
    setStatus(doneAfterStep() ? CStatus::Done : CStatus::Updated);
}

void AtenModelComponent::finish()
{
    setStatus(CStatus::Finishing);
    m_haveStep = false;
    m_point = Point{};
    setStatus(CStatus::Finished);
}

std::set<HydroCouple::Capability> AtenModelComponent::capabilities() const
{
    std::set<HydroCouple::Capability> caps = AbstractModelComponent::capabilities();
    caps.insert(HydroCouple::Capability::Checkpointing);
    caps.insert(HydroCouple::Capability::Differentiable);
    return caps;
}

void AtenModelComponent::afterStep(const StepResult &) {}

bool AtenModelComponent::doneAfterStep() const { return false; }

int AtenModelComponent::stepIndex() const noexcept { return m_stepIndex; }

int AtenModelComponent::completedSteps() const noexcept { return m_steps; }

// ============================================================================
// Gathering and checking
// ============================================================================

bool AtenModelComponent::gather(Point &point, std::string &message) const
{
    auto read = [&message](const IComponentDataItem *item, std::vector<torch::Tensor> &into) {
        torch::Tensor t = readItem(item, &message);
        if (!t.defined())
            return false;
        into.push_back(std::move(t));
        return true;
    };
    for (IComponentDataItem *item : differentiableStates())
        if (!read(item, point.states))
            return false;
    for (HydroCouple::IInput *item : differentiableInputs())
        if (!read(item, point.inputs))
            return false;
    for (HydroCouple::IArgument *item : differentiableArguments())
        if (!read(item, point.arguments))
            return false;
    return true;
}

bool AtenModelComponent::checkResult(const StepResult &result, std::string &message) const
{
    const auto states = differentiableStates();
    const auto outputs = differentiableOutputs();
    if (result.states.size() != states.size())
        return fail(&message, "step() returned " + std::to_string(result.states.size()) +
                                  " states; differentiableStates() lists " +
                                  std::to_string(states.size()));
    if (result.outputs.size() != outputs.size())
        return fail(&message, "step() returned " + std::to_string(result.outputs.size()) +
                                  " outputs; differentiableOutputs() lists " +
                                  std::to_string(outputs.size()));
    auto check = [&message](const IComponentDataItem *item, const torch::Tensor &t) {
        if (!t.defined())
            return fail(&message, "step() returned no tensor for \"" + item->id() + "\"");
        if (shapeOf(t) != item->shape())
            return fail(&message, "step() returned shape " + shapeString(shapeOf(t)) +
                                      " for \"" + item->id() + "\" of shape " +
                                      shapeString(item->shape()));
        return true;
    };
    for (size_t i = 0; i < states.size(); ++i)
        if (!check(states[i], result.states[i]))
            return false;
    for (size_t i = 0; i < outputs.size(); ++i)
        if (!check(outputs[i], result.outputs[i]))
            return false;
    return true;
}

bool AtenModelComponent::relinearize(std::vector<torch::Tensor> &leaves, StepResult &result,
                                     std::string &message)
{
    if (!m_haveStep)
        return fail(&message, "no step to differentiate: component \"" + id() +
                                  "\" has not updated since it was prepared or restored");
    std::vector<torch::Tensor> states, inputs, arguments;
    auto leaf = [&leaves](const torch::Tensor &t, std::vector<torch::Tensor> &into) {
        torch::Tensor l = t.detach().clone().requires_grad_(true);
        leaves.push_back(l);
        into.push_back(l);
    };
    leaves.clear();
    for (const auto &t : m_point.states)
        leaf(t, states);
    for (const auto &t : m_point.inputs)
        leaf(t, inputs);
    for (const auto &t : m_point.arguments)
        leaf(t, arguments);
    const int saved = m_stepIndex;
    try
    {
        torch::AutoGradMode enable(true);
        m_stepIndex = m_point.step;
        result = step(states, inputs, arguments);
        m_stepIndex = saved;
    }
    catch (const std::exception &e)
    {
        m_stepIndex = saved;
        return fail(&message, std::string("step() threw: ") + e.what());
    }
    return checkResult(result, message);
}

// ============================================================================
// vjp / jvp
// ============================================================================

namespace {

//! Where an entry of a given role lives among the leaves (states, inputs,
//! arguments) of a relinearized step; -1 if it is not one of them.
template <typename Component>
int leafIndex(const Component &c, const DifferentialEntry &e, size_t nStates, size_t nInputs)
{
    int i = -1;
    switch (e.role)
    {
    case DifferentialRole::StateBefore:
        i = indexOf(c.differentiableStates(), e.item);
        return i;
    case DifferentialRole::Input:
        i = indexOf(c.differentiableInputs(), e.item);
        return i < 0 ? -1 : static_cast<int>(nStates) + i;
    case DifferentialRole::Argument:
        i = indexOf(c.differentiableArguments(), e.item);
        return i < 0 ? -1 : static_cast<int>(nStates + nInputs) + i;
    default:
        return -1;
    }
}

//! Where an entry lives among the results (states', then outputs).
template <typename Component>
int resultIndex(const Component &c, const DifferentialEntry &e, size_t nStates)
{
    int i = -1;
    switch (e.role)
    {
    case DifferentialRole::StateAfter:
        return indexOf(c.differentiableStates(), e.item);
    case DifferentialRole::Output:
        i = indexOf(c.differentiableOutputs(), e.item);
        return i < 0 ? -1 : static_cast<int>(nStates) + i;
    default:
        return -1;
    }
}

std::string entryName(const DifferentialEntry &e)
{
    return e.item ? "\"" + e.item->id() + "\"" : std::string("(null item)");
}

} // namespace

bool AtenModelComponent::vjp(DifferentialSet seeds, DifferentialSet results, std::string *message)
{
    std::string msg;
    std::vector<torch::Tensor> leaves;
    StepResult result;
    if (!relinearize(leaves, result, msg))
        return fail(message, msg);

    const size_t nStates = m_point.states.size();
    const size_t nInputs = m_point.inputs.size();
    std::vector<torch::Tensor> produced = result.states;
    produced.insert(produced.end(), result.outputs.begin(), result.outputs.end());

    std::vector<torch::Tensor> roots, cotangents;
    for (const DifferentialEntry &e : seeds)
    {
        const int r = resultIndex(*this, e, nStates);
        if (r < 0)
            return fail(message, "vjp(): " + entryName(e) +
                                     " is not a differentiable output or state of \"" + id() + "\"");
        torch::Tensor seed;
        if (!readEntry(e, shapeOf(produced[r]), seed, message))
            return false;
        // An output that no leaf reaches has no graph; its seed contributes nothing.
        if (!produced[r].requires_grad())
            continue;
        roots.push_back(produced[r]);
        cotangents.push_back(seed);
    }

    std::vector<torch::Tensor> grads(leaves.size());
    if (!roots.empty())
    {
        try
        {
            grads = torch::autograd::grad(roots, leaves, cotangents,
                                          /*retain_graph=*/false, /*create_graph=*/false,
                                          /*allow_unused=*/true);
        }
        catch (const std::exception &e)
        {
            return fail(message, std::string("vjp(): autograd failed: ") + e.what());
        }
    }

    for (const DifferentialEntry &e : results)
    {
        const int l = leafIndex(*this, e, nStates, nInputs);
        if (l < 0)
            return fail(message, "vjp(): " + entryName(e) +
                                     " is not a differentiable input, argument or state of \"" +
                                     id() + "\"");
        if (!writeEntry(e, shapeOf(leaves[l]), grads[l], message))
            return false;
    }
    return true;
}

bool AtenModelComponent::jvp(DifferentialSet seeds, DifferentialSet results, std::string *message)
{
    std::string msg;
    std::vector<torch::Tensor> leaves;
    StepResult result;
    if (!relinearize(leaves, result, msg))
        return fail(message, msg);

    const size_t nStates = m_point.states.size();
    const size_t nInputs = m_point.inputs.size();
    std::vector<torch::Tensor> produced = result.states;
    produced.insert(produced.end(), result.outputs.begin(), result.outputs.end());

    // Tangents per leaf (undefined: zero).
    std::vector<torch::Tensor> tangents(leaves.size());
    for (const DifferentialEntry &e : seeds)
    {
        const int l = leafIndex(*this, e, nStates, nInputs);
        if (l < 0)
            return fail(message, "jvp(): " + entryName(e) +
                                     " is not a differentiable input, argument or state of \"" +
                                     id() + "\"");
        torch::Tensor t;
        if (!readEntry(e, shapeOf(leaves[l]), t, message))
            return false;
        tangents[l] = tangents[l].defined() ? tangents[l] + t : t;
    }

    // Double backward: g(u) = J^T u is linear in u, so d<g(u), t>/du = J t.
    std::vector<torch::Tensor> jt(produced.size());
    try
    {
        std::vector<torch::Tensor> roots, dummies;
        std::vector<size_t> rootOf;
        for (size_t r = 0; r < produced.size(); ++r)
            if (produced[r].requires_grad())
            {
                roots.push_back(produced[r]);
                dummies.push_back(torch::zeros_like(produced[r]).requires_grad_(true));
                rootOf.push_back(r);
            }
        if (!roots.empty())
        {
            std::vector<torch::Tensor> g = torch::autograd::grad(
                roots, leaves, dummies, /*retain_graph=*/true, /*create_graph=*/true,
                /*allow_unused=*/true);
            std::vector<torch::Tensor> gs, ts;
            for (size_t l = 0; l < leaves.size(); ++l)
                if (g[l].defined() && g[l].requires_grad() && tangents[l].defined())
                {
                    gs.push_back(g[l]);
                    ts.push_back(tangents[l]);
                }
            if (!gs.empty())
            {
                std::vector<torch::Tensor> j = torch::autograd::grad(
                    gs, dummies, ts, /*retain_graph=*/false, /*create_graph=*/false,
                    /*allow_unused=*/true);
                for (size_t k = 0; k < rootOf.size(); ++k)
                    jt[rootOf[k]] = j[k];
            }
        }
    }
    catch (const std::exception &e)
    {
        return fail(message, std::string("jvp(): autograd failed: ") + e.what());
    }

    for (const DifferentialEntry &e : results)
    {
        const int r = resultIndex(*this, e, nStates);
        if (r < 0)
            return fail(message, "jvp(): " + entryName(e) +
                                     " is not a differentiable output or state of \"" + id() + "\"");
        if (!writeEntry(e, shapeOf(produced[r]), jt[r], message))
            return false;
    }
    return true;
}

// ============================================================================
// Checkpoints: the token is the state
// ============================================================================

namespace {
constexpr const char *kTokenTag = "aten1";
}

bool AtenModelComponent::saveState(std::string &token, std::string &message)
{
    std::string out = kTokenTag;
    char buffer[40];
    std::snprintf(buffer, sizeof buffer, " %d", m_steps);
    out += buffer;
    auto append = [&](const IComponentDataItem *item) {
        torch::Tensor t = readItem(item, &message);
        if (!t.defined())
            return false;
        std::snprintf(buffer, sizeof buffer, " %lld", static_cast<long long>(t.numel()));
        out += buffer;
        const double *v = t.data_ptr<double>();
        for (int64_t i = 0; i < t.numel(); ++i)
        {
            std::snprintf(buffer, sizeof buffer, " %.17g", v[i]);
            out += buffer;
        }
        return true;
    };
    for (IComponentDataItem *item : differentiableStates())
        if (!append(item))
            return false;
    for (HydroCouple::IOutput *item : outputs())
        if (item->dataKind() == DataKind::Float64 && !append(item))
            return false;
    token = std::move(out);
    return true;
}

bool AtenModelComponent::restoreState(const std::string &token, std::string &message)
{
    const char *p = token.c_str();
    char *end = nullptr;
    if (token.rfind(kTokenTag, 0) != 0)
        return fail(&message, "not a checkpoint of an AtenModelComponent");
    p += std::char_traits<char>::length(kTokenTag);
    const long steps = std::strtol(p, &end, 10);
    if (end == p)
        return fail(&message, "bad checkpoint token");
    p = end;

    // Parse everything before writing anything, so a bad token changes nothing.
    std::vector<std::pair<IComponentDataItem *, torch::Tensor>> writes;
    auto take = [&](IComponentDataItem *item) {
        const long long n = std::strtoll(p, &end, 10);
        const std::vector<int64_t> shape = item->shape();
        if (end == p || n != elementCount(shape))
            return fail(&message, "bad checkpoint token for \"" + item->id() + "\"");
        p = end;
        torch::Tensor t = torch::empty(shape, torch::kFloat64);
        double *v = t.data_ptr<double>();
        for (long long i = 0; i < n; ++i)
        {
            v[i] = std::strtod(p, &end);
            if (end == p)
                return fail(&message, "bad checkpoint token for \"" + item->id() + "\"");
            p = end;
        }
        writes.emplace_back(item, std::move(t));
        return true;
    };
    for (IComponentDataItem *item : differentiableStates())
        if (!take(item))
            return false;
    for (HydroCouple::IOutput *item : outputs())
        if (item->dataKind() == DataKind::Float64 && !take(item))
            return false;
    for (auto &[item, values] : writes)
        if (!writeItem(item, values, &message))
            return false;

    m_steps = static_cast<int>(steps);
    m_stepIndex = m_steps;
    m_haveStep = false;
    m_point = Point{};
    setStatus(CStatus::Updated);
    message.clear();
    return true;
}

bool AtenModelComponent::releaseState(const std::string &, std::string &message)
{
    message.clear();
    return true;
}

} // namespace HydroCouple::SDK::Differentiation::Torch
