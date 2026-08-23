// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   argument2d.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Concrete 2-D typed IArgument implementation.
 *
 * Argument2D<T> combines AbstractArgument with ComponentDataItem2D<T> and
 * implements the v2 typed hyperslab data plane by delegating to the storage
 * mixin. Values serialise as a rectangular JSON array-of-arrays (rows);
 * initialize() accepts that form directly, an object with a "values" key,
 * or a file/URL containing either.
 */

#ifndef HYDROCOUPLESDK_ARGUMENT2D_H
#define HYDROCOUPLESDK_ARGUMENT2D_H

#include "hydrocouplesdk/data/abstractargument.h"
#include "hydrocouplesdk/data/componentdataitem.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace HydroCouple::SDK {

/*!
 * \brief Concrete 2-D typed argument (row-major storage).
 * \tparam T  Element type (int, double, std::string, …).
 */
template <typename T>
class Argument2D
    : public AbstractArgument,
      public ComponentDataItem2D<T>
{
public:
    /*!
     * \brief Construct with identifier, dimensions, initial extents, value
     *        definition, and owning component.
     */
    Argument2D(std::string_view id,
               Dimension *dimensionX, Dimension *dimensionY,
               int lengthX, int lengthY,
               ValueDefinition *valueDef, HydroCouple::IModelComponent *mc)
        : AbstractArgument(id, {dimensionX, dimensionY}, valueDef, mc),
          ComponentDataItem2D<T>(lengthX, lengthY),
          m_dimensionX(dimensionX),
          m_dimensionY(dimensionY)
    {}

    ~Argument2D() override = default;

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return ComponentDataItem2D<T>::storageShape();
    }

    [[nodiscard]] DataKind dataKind() const override
    {
        return ComponentDataItem2D<T>::storageKind();
    }

    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override
    {
        return ComponentDataItem2D<T>::getSlab(destination, start, count, message);
    }

    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override
    {
        if (!ComponentDataItem2D<T>::setSlab(source, start, count, message))
            return false;
        emitValueChanged({start.begin(), start.end()},
                         {count.begin(), count.end()});
        return true;
    }

    // ── IArgument ─────────────────────────────────────────────────────────

    /*!
     * \brief Returns the values as a JSON array-of-arrays string (rows).
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
                m_inputFile = value;
                std::ifstream f(value);
                if (!f) { message = "Cannot open file: " + value; return false; }
                std::ostringstream ss;
                ss << f.rdbuf();
                text = ss.str();
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
     * \brief Copies values from another 2-D data item of the same DataKind.
     */
    [[nodiscard]] bool initialize(const HydroCouple::IComponentDataItem &componentDataItem,
                                  std::string &message) override
    {
        const auto srcShape = componentDataItem.shape();
        if (srcShape.size() != 2)
        {
            message = "Source data item is not two-dimensional";
            return false;
        }
        if (componentDataItem.dataKind() != ComponentDataItem2D<T>::storageKind())
        {
            message = "Source data kind mismatch";
            return false;
        }
        if (srcShape[0] != this->iLength() || srcShape[1] != this->jLength())
            this->resizeDataArray(static_cast<int>(srcShape[0]),
                                  static_cast<int>(srcShape[1]));
        const BufferDescriptor dest = detail::hostDescriptor(
            this->rawData().data(), ComponentDataItem2D<T>::storageKind(),
            std::span<const int64_t>(srcShape.data(), 2));
        const std::array<int64_t, 2> zero{0, 0};
        return componentDataItem.getValuesInto(
            dest, std::span<const int64_t>(zero.data(), 2),
            std::span<const int64_t>(srcShape.data(), 2), &message);
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
        auto rows = nlohmann::json::array();
        for (int i = 0; i < this->iLength(); ++i)
        {
            auto row = nlohmann::json::array();
            for (int jj = 0; jj < this->jLength(); ++jj)
                row.push_back((*this)(i, jj));
            rows.push_back(std::move(row));
        }
        return rows;
    }

    bool assignValues(const nlohmann::json &j, std::string &message)
    {
        if (!j.is_array())
        {
            message = "Expected JSON array of row arrays";
            return false;
        }
        const int rows = static_cast<int>(j.size());
        const int cols = rows > 0 ? static_cast<int>(j[0].size()) : 0;
        for (const auto &row : j)
        {
            if (!row.is_array() || static_cast<int>(row.size()) != cols)
            {
                message = "Rows must be arrays of equal length";
                return false;
            }
        }
        this->resizeDataArray(rows, cols);
        for (int i = 0; i < rows; ++i)
            for (int jj = 0; jj < cols; ++jj)
                (*this)(i, jj) = j[i][jj].get<T>();
        emitValueChanged({0, 0}, {static_cast<int64_t>(rows),
                                  static_cast<int64_t>(cols)});
        return true;
    }

    Dimension             *m_dimensionX;
    Dimension             *m_dimensionY;
    std::filesystem::path  m_inputFile;
};

using Argument2DInt    = Argument2D<int>;
using Argument2DDouble = Argument2D<double>;
using Argument2DString = Argument2D<std::string>;

extern template class HYDROCOUPLESDK_EXPORT Argument2D<int>;
extern template class HYDROCOUPLESDK_EXPORT Argument2D<double>;
extern template class HYDROCOUPLESDK_EXPORT Argument2D<std::string>;

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ARGUMENT2D_H
