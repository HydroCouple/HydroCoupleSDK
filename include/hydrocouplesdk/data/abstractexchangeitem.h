// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   abstractexchangeitem.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Abstract base implementation of HydroCouple::IExchangeItem.
 */

#ifndef HYDROCOUPLESDK_ABSTRACTEXCHANGEITEM_H
#define HYDROCOUPLESDK_ABSTRACTEXCHANGEITEM_H

#include "hydrocouplesdk/data/abstractcomponentdataitem.h"
#include "hydrocouple.h"

#include <memory>
#include <string>

namespace HydroCouple::SDK {

// ── AbstractExchangeItem ──────────────────────────────────────────────────

/*!
 * \brief Abstract base class for HydroCouple::IExchangeItem.
 *
 * Extends AbstractComponentDataItem with a helper emitMessage() used by
 * subclasses to fire value-changed notifications with a status message.
 *
 * IExchangeItem itself adds no extra pure virtuals beyond IComponentDataItem;
 * the concrete data is added by IInput/IOutput subclasses.
 *
 * \threadsafety Signal connections are thread-safe (std::shared_mutex);
 * data mutation is not.
 */
class HYDROCOUPLESDK_EXPORT AbstractExchangeItem
    : public AbstractComponentDataItem,
      public virtual HydroCouple::IExchangeItem
{
public:
    AbstractExchangeItem(std::string_view id,
                          const std::vector<Dimension *> &dimensions,
                          ValueDefinition *valueDefinition,
                          HydroCouple::IModelComponent *modelComponent);

    AbstractExchangeItem(std::string_view id,
                          std::string_view caption,
                          const std::vector<Dimension *> &dimensions,
                          ValueDefinition *valueDefinition,
                          HydroCouple::IModelComponent *modelComponent);

    ~AbstractExchangeItem() override = default;

protected:
    /*!
     * \brief Fires the itemChanged signal with a status message.
     * \param message  Human-readable description of the change.
     */
    void emitMessage(const std::string &message);
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTEXCHANGEITEM_H
