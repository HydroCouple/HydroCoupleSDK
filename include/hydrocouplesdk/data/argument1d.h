// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   argument1d.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Concrete 1-D typed IArgument implementation.
 *
 * Argument1D<T> combines AbstractArgument with ComponentDataItem1D<T> and
 * implements the v2 typed hyperslab data plane by delegating to the storage
 * mixin. Values serialise as a JSON array; initialize() accepts a bare JSON
 * array, an object with a "values" key, or a file/URL containing either.
 */

#ifndef HYDROCOUPLESDK_ARGUMENT1D_H
#define HYDROCOUPLESDK_ARGUMENT1D_H

#include "hydrocouplesdk/data/abstractargument.h"
#include "hydrocouplesdk/io/uriresolver.h"
#include "hydrocouplesdk/data/componentdataitem.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace HydroCouple::SDK {

/*!
 * \brief Concrete 1-D typed argument.
 * \tparam T  Element type (int, double, std::string, …).
 */
template <typename T>
class Argument1D
    : public AbstractArgument,
      public ComponentDataItem1D<T>
{
public:
    /*!
     * \brief Construct with identifier, dimension, initial length, value
     *        definition, and owning component.
     */
    Argument1D(std::string_view id, Dimension *dimension, int length,
               ValueDefinition *valueDef, HydroCouple::IModelComponent *mc)
        : AbstractArgument(id, {dimension}, valueDef, mc),
          ComponentDataItem1D<T>(length),
          m_dimension(dimension)
    {}

    ~Argument1D() override = default;

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return ComponentDataItem1D<T>::storageShape();
    }

    [[nodiscard]] DataKind dataKind() const override
    {
        return ComponentDataItem1D<T>::storageKind();
    }

    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override
    {
        return ComponentDataItem1D<T>::getSlab(destination, start, count, message);
    }

    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override
    {
        if (!ComponentDataItem1D<T>::setSlab(source, start, count, message))
            return false;
        emitValueChanged({start.begin(), start.end()},
                         {count.begin(), count.end()});
        return true;
    }

    // ── IArgument ─────────────────────────────────────────────────────────

    /*!
     * \brief Returns the values as a JSON array string.
     */
    [[nodiscard]] std::string toString() const override
    {
        return valuesToJson().dump();
    }

    /*!
     * \brief Writes the current values back to the input file, if one was
     *        set via File/URL initialisation.
     */
    void saveData() override
    {
        if (m_inputFile.empty())
            return;
        std::ofstream f(m_inputFile);
        if (f)
            f << toString();
    }

    /*!
     * \brief Parses \p value according to \p argType and stores the result.
     */
    [[nodiscard]] bool initialize(const std::string &value,
                                  HydroCouple::IArgument::ArgumentInputType argType,
                                  std::string &message) override
    {
        using AT = HydroCouple::IArgument::ArgumentInputType;
        try
        {
            std::string text = value;
            if (argType == AT::File || argType == AT::URL)
            {
                if (!IO::readReference(value, text, message))
                    return false;
                // Only something on disk can be written back to; remembering
                // a URL here would make saveData() create a file named after
                // a service.
                m_inputFile = IO::isWritableReference(value)
                                  ? std::filesystem::path(value)
                                  : std::filesystem::path();
            }
            auto j = nlohmann::json::parse(text);
            if (j.is_object() && j.contains("values"))
                j = j["values"];
            if (!assignValues(j, message))
                return false;
            setCurrentArgumentInputType(argType);
            return true;
        }
        catch (const std::exception &e)
        {
            message = e.what();
            return false;
        }
    }

    /*!
     * \brief Copies values from another 1-D data item of the same DataKind.
     */
    [[nodiscard]] bool initialize(const HydroCouple::IComponentDataItem &componentDataItem,
                                  std::string &message) override
    {
        const auto srcShape = componentDataItem.shape();
        if (srcShape.size() != 1)
        {
            message = "Source data item is not one-dimensional";
            return false;
        }
        if (componentDataItem.dataKind() != ComponentDataItem1D<T>::storageKind())
        {
            message = "Source data kind mismatch";
            return false;
        }
        const int64_t n = srcShape[0];
        if (n != this->length())
            this->resizeDataArray(static_cast<int>(n));
        const BufferDescriptor dest = detail::hostDescriptor(
            this->rawData().data(), ComponentDataItem1D<T>::storageKind(),
            std::span<const int64_t>(&n, 1));
        const int64_t zero = 0;
        return componentDataItem.getValuesInto(
            dest, std::span<const int64_t>(&zero, 1),
            std::span<const int64_t>(&n, 1), &message);
    }

    // ── JSON persistence ──────────────────────────────────────────────────

    void readData(const nlohmann::json &j) override
    {
        AbstractArgument::readData(j);
        if (j.contains("values"))
        {
            std::string msg;
            (void)assignValues(j["values"], msg);
        }
    }

    [[nodiscard]] nlohmann::json writeData() const override
    {
        auto j = AbstractArgument::writeData();
        j["values"] = valuesToJson();
        return j;
    }

private:
    [[nodiscard]] nlohmann::json valuesToJson() const
    {
        auto arr = nlohmann::json::array();
        for (int i = 0; i < this->length(); ++i)
            arr.push_back((*this)[i]);
        return arr;
    }

    bool assignValues(const nlohmann::json &j, std::string &message)
    {
        if (!j.is_array())
        {
            message = "Expected JSON array of values";
            return false;
        }
        this->resizeDataArray(static_cast<int>(j.size()));
        for (int i = 0; i < static_cast<int>(j.size()); ++i)
            (*this)[i] = j[i].get<T>();
        emitValueChanged({0}, {static_cast<int64_t>(j.size())});
        return true;
    }

    Dimension             *m_dimension;
    std::filesystem::path  m_inputFile;
};

using Argument1DInt    = Argument1D<int>;
using Argument1DDouble = Argument1D<double>;
using Argument1DString = Argument1D<std::string>;

extern template class HYDROCOUPLESDK_EXPORT Argument1D<int>;
extern template class HYDROCOUPLESDK_EXPORT Argument1D<double>;
extern template class HYDROCOUPLESDK_EXPORT Argument1D<std::string>;

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ARGUMENT1D_H
