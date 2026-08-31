// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   idbasedargument.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Concrete ID-based typed IArgument implementation.
 *
 * IdBasedArgument<T> combines AbstractArgument, IdBasedComponentDataItem<T>,
 * and IIdBasedComponentDataItem, implementing the v2 typed hyperslab data
 * plane with the identifier dimension as dimension 0.
 *
 * Values serialise as a JSON object mapping identifier to value
 * (\c {"nodeA": 1.5, "nodeB": 2.0}); initialize() accepts that form,
 * an object with "identifiers"/"values" arrays, or a file/URL containing
 * either.
 */

#ifndef HYDROCOUPLESDK_IDBASEDARGUMENT_H
#define HYDROCOUPLESDK_IDBASEDARGUMENT_H

#include "hydrocouplesdk/data/abstractargument.h"
#include "hydrocouplesdk/io/uriresolver.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/data/idbasedcomponentdataitem.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace HydroCouple::SDK {

/*!
 * \brief Concrete ID-keyed typed argument.
 * \tparam T  Element type (int, double, std::string, …).
 */
template <typename T>
class IdBasedArgument
    : public AbstractArgument,
      public IdBasedComponentDataItem<T>,
      public virtual HydroCouple::IIdBasedComponentDataItem
{
public:
    /*!
     * \brief Construct with identifier, id list, identifier dimension,
     *        value definition, and owning component.
     */
    IdBasedArgument(std::string_view id,
                    const std::vector<std::string> &identifiers,
                    Dimension *identifierDimension,
                    ValueDefinition *valueDef,
                    HydroCouple::IModelComponent *mc)
        : AbstractArgument(id, {identifierDimension}, valueDef, mc),
          IdBasedComponentDataItem<T>(identifiers),
          m_identifierDimension(identifierDimension)
    {}

    ~IdBasedArgument() override = default;

    // ── IIdBasedComponentDataItem ────────────────────────────────────────

    [[nodiscard]] std::vector<std::string> identifiers() const override
    {
        return IdBasedComponentDataItem<T>::identifiers();
    }

    [[nodiscard]] HydroCouple::IDimension *identifierDimension() const override
    {
        return m_identifierDimension;
    }

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return IdBasedComponentDataItem<T>::storageShape();
    }

    [[nodiscard]] DataKind dataKind() const override
    {
        return IdBasedComponentDataItem<T>::storageKind();
    }

    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override
    {
        return IdBasedComponentDataItem<T>::getSlab(destination, start, count,
                                                    message);
    }

    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override
    {
        if (!IdBasedComponentDataItem<T>::setSlab(source, start, count, message))
            return false;
        emitValueChanged({start.begin(), start.end()},
                         {count.begin(), count.end()});
        return true;
    }

    // ── IArgument ─────────────────────────────────────────────────────────

    /*!
     * \brief Returns the values as a JSON id→value object string.
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
            if (!assignValues(nlohmann::json::parse(text), message))
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
     * \brief Copies identifiers and values from another id-based data item
     *        of the same DataKind.
     */
    [[nodiscard]] bool initialize(const HydroCouple::IComponentDataItem &componentDataItem,
                                  std::string &message) override
    {
        const auto *src = dynamic_cast<const HydroCouple::IIdBasedComponentDataItem *>(
            &componentDataItem);
        if (!src)
        {
            message = "Source data item is not id-based";
            return false;
        }
        if (componentDataItem.dataKind() != IdBasedComponentDataItem<T>::storageKind())
        {
            message = "Source data kind mismatch";
            return false;
        }
        this->setIdentifiers(src->identifiers());
        const int64_t n = static_cast<int64_t>(this->length());
        if (n == 0)
            return true;
        const BufferDescriptor dest = detail::hostDescriptor(
            this->rawData().data(), IdBasedComponentDataItem<T>::storageKind(),
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
            nlohmann::json payload = j;
            (void)assignValues(payload, msg);
        }
    }

    [[nodiscard]] nlohmann::json writeData() const override
    {
        auto j = AbstractArgument::writeData();
        j["identifiers"] = IdBasedComponentDataItem<T>::identifiers();
        auto arr = nlohmann::json::array();
        for (int i = 0; i < this->length(); ++i)
            arr.push_back((*this)[i]);
        j["values"] = arr;
        return j;
    }

private:
    [[nodiscard]] nlohmann::json valuesToJson() const
    {
        auto obj = nlohmann::json::object();
        const auto ids = IdBasedComponentDataItem<T>::identifiers();
        for (int i = 0; i < this->length(); ++i)
            obj[ids[i]] = (*this)[i];
        return obj;
    }

    /*!
     * Accepts {"id": value, ...} or {"identifiers": [...], "values": [...]}.
     */
    bool assignValues(const nlohmann::json &j, std::string &message)
    {
        if (!j.is_object())
        {
            message = "Expected JSON object";
            return false;
        }
        if (j.contains("identifiers") && j.contains("values"))
        {
            const auto &ids  = j["identifiers"];
            const auto &vals = j["values"];
            if (!ids.is_array() || !vals.is_array() || ids.size() != vals.size())
            {
                message = "identifiers/values must be arrays of equal length";
                return false;
            }
            this->setIdentifiers(ids.get<std::vector<std::string>>());
            for (int i = 0; i < static_cast<int>(vals.size()); ++i)
                (*this)[i] = vals[i].get<T>();
        }
        else
        {
            std::vector<std::string> ids;
            ids.reserve(j.size());
            for (const auto &[key, val] : j.items())
                ids.push_back(key);
            this->setIdentifiers(ids);
            for (const auto &[key, val] : j.items())
                this->valueAt(key) = val.template get<T>();
        }
        emitValueChanged({0}, {static_cast<int64_t>(this->length())});
        return true;
    }

    Dimension             *m_identifierDimension;
    std::filesystem::path  m_inputFile;
};

using IdBasedArgumentInt    = IdBasedArgument<int>;
using IdBasedArgumentDouble = IdBasedArgument<double>;
using IdBasedArgumentString = IdBasedArgument<std::string>;

extern template class HYDROCOUPLESDK_EXPORT IdBasedArgument<int>;
extern template class HYDROCOUPLESDK_EXPORT IdBasedArgument<double>;
extern template class HYDROCOUPLESDK_EXPORT IdBasedArgument<std::string>;

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_IDBASEDARGUMENT_H
