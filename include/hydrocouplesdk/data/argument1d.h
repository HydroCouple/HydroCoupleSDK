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
 * mixin. Values serialize as a JSON array; initialize() accepts a bare JSON
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
 *
 * \details The usual argument of a component: a list of numbers or strings,
 * for example a coefficient per reach or a single scalar setting stored as a
 * list of length 1. Its shape is { length() } and length() follows the last
 * value read, so an argument constructed with length 0 takes the size of
 * whatever the host gives it.
 *
 * Accepted input (initialize() with a string):
 * | Input type | \p value holds |
 * | --- | --- |
 * | String, JSON (and XML, YAML, MEMORY_OBJECT, which are read the same way) | JSON text: an array such as `[1.5, 2.5]`, or an object with a "values" array |
 * | File | a path or `file:` URI of a file holding that JSON; saveData() writes back to it |
 * | URL | a URI read through IO::readReference(); a remote one is recorded as sourceUri() |
 *
 * Elements are converted with nlohmann::json::get of \c T, so a string in a
 * double argument fails with the library's type_error text. Every successful
 * read fires value-changed for the whole array.
 *
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
     * \param id       Argument identifier, unique within the component.
     * \param dimension The argument's single axis (borrowed).
     * \param length   Initial element count; elements start at \c T{}.
     * \param valueDef Value definition (borrowed; may be nullptr).
     * \param mc       Owning component (borrowed; may be nullptr).
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
     * \return Compact JSON, for example `[1.5,2.5]`.
     */
    [[nodiscard]] std::string toString() const override
    {
        return valuesToJson().dump();
    }

    /*!
     * \brief Writes the current values back to the input file, if one was
     *        set via File/URL initialization.
     * \details Only a local path or \c file: reference is remembered; a
     *          remote URL is not written back. The file is overwritten with
     *          toString() (a bare array, even if it held an object before).
     *          A file that cannot be opened is skipped silently.
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
     * \details See the class table for the accepted forms. On failure the
     *          message is the reader's (`Cannot open file: PATH`, or
     *          `No resolver is installed for 'SCHEME' URIs, so this argument cannot read URI`),
     *          the JSON parser's, or "Expected JSON array of values"; the
     *          previous values are kept unless the failure came from an
     *          element conversion part way through.
     * \param[in]  value   JSON text, or a path or URI for File and URL.
     * \param[in]  argType Representation of \p value.
     * \param[out] message Failure description.
     * \return true when the values were read; currentArgumentInputType()
     *         is then \p argType (URL for a payload that named a URI).
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
                // a service. By the same split, only a remote reference is
                // worth recording: a file's values can be inlined, a
                // service's cannot.
                const bool local = IO::isWritableReference(value);
                m_inputFile = local ? std::filesystem::path(value)
                                    : std::filesystem::path();
                setSourceUri(local ? std::string() : value);
            }
            else if (std::string uri; IO::jsonUriReference(text, uri))
            {
                // A payload that names a URI is that URI, read now. This is
                // what carries a fetched argument across a save and an open.
                return initialize(uri, AT::URL, message);
            }
            else
            {
                // Values given outright are their own provenance.
                setSourceUri({});
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
     * \details Resizes to the source's length and reads all of it with
     *          getValuesInto(). Refuses a source that is not rank 1 ("Source
     *          data item is not one-dimensional") or of another kind ("Source
     *          data kind mismatch"). Fires no value-changed signal and leaves
     *          currentArgumentInputType() unchanged.
     * \param[in]  componentDataItem Source item, for example another
     *                               component's output.
     * \param[out] message           Failure description.
     * \return true when the values were copied.
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

    /*!
     * \brief Reads the base fields and, when present, the "values" array.
     * \details A "values" entry that is not an array is ignored without a
     *          message; an element of the wrong type throws
     *          nlohmann::json::type_error.
     * \param[in] j JSON object as written by writeData().
     */
    void readData(const nlohmann::json &j) override
    {
        AbstractArgument::readData(j);
        if (j.contains("values"))
        {
            std::string msg;
            (void)assignValues(j["values"], msg);
        }
    }

    /*!
     * \brief Writes the base fields and the values.
     * \return The AbstractArgument object with "values" holding the array.
     */
    [[nodiscard]] nlohmann::json writeData() const override
    {
        auto j = AbstractArgument::writeData();
        j["values"] = valuesToJson();
        return j;
    }

private:
    //! The values as a JSON array.
    //! \return One JSON element per value, in index order.
    [[nodiscard]] nlohmann::json valuesToJson() const
    {
        auto arr = nlohmann::json::array();
        for (int i = 0; i < this->length(); ++i)
            arr.push_back((*this)[i]);
        return arr;
    }

    /*!
     * \brief Replaces the values with the elements of a JSON array.
     * \details Resizes to the array's size and fires value-changed for the
     *          whole array.
     * \param[in]  j       The array.
     * \param[out] message "Expected JSON array of values" when \p j is not
     *                     an array.
     * \return false when \p j is not an array.
     * \exception nlohmann::json::type_error when an element does not convert
     *            to \c T (the array has already been resized).
     */
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

    Dimension             *m_dimension; //!< The argument's axis (borrowed).
    std::filesystem::path  m_inputFile; //!< Local file saveData() writes to, or empty.
};

using Argument1DInt    = Argument1D<int>;         //!< 1-D argument of \c int (DataKind::Int32).
using Argument1DDouble = Argument1D<double>;      //!< 1-D argument of \c double (DataKind::Float64).
using Argument1DString = Argument1D<std::string>; //!< 1-D argument of \c std::string (DataKind::String, host only).

extern template class HYDROCOUPLESDK_EXPORT Argument1D<int>;
extern template class HYDROCOUPLESDK_EXPORT Argument1D<double>;
extern template class HYDROCOUPLESDK_EXPORT Argument1D<std::string>;

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ARGUMENT1D_H
