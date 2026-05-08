// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   abstractexchangeitem.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Abstract base implementation of HydroCouple::IExchangeItem.
 */

#ifndef HYDROCOUPLESDK_ABSTRACTEXCHANGEITEM_H
#define HYDROCOUPLESDK_ABSTRACTEXCHANGEITEM_H

#include "abstractcomponentdataitem.h"
#include "hydrocouple.h"

#include <memory>
#include <string>

namespace HydroCouple {

// ── ExchangeItemChangeEventArgs ───────────────────────────────────────────

/*!
 * \brief Concrete implementation of HydroCouple::IExchangeItemChangeEventArgs.
 *
 * Carries a pointer to the exchange item that fired the event and a
 * human-readable message describing the change.
 */
class HYDROCOUPLESDK_EXPORT ExchangeItemChangeEventArgs
    : public HydroCouple::IExchangeItemChangeEventArgs
{
public:
    ExchangeItemChangeEventArgs(HydroCouple::IExchangeItem *item,
                                 std::string message = {});

    ~ExchangeItemChangeEventArgs() override = default;

    [[nodiscard]] HydroCouple::IExchangeItem *exchangeItem() const override;
    [[nodiscard]] std::string                 message()      const override;

private:
    HydroCouple::IExchangeItem *m_item;
    std::string                 m_message;
};


// ── AbstractExchangeItem ──────────────────────────────────────────────────

/*!
 * \brief Abstract base class for HydroCouple::IExchangeItem.
 *
 * Extends AbstractComponentDataItem with the exchange-item signal
 * (ISignal<const shared_ptr<IExchangeItemChangeEventArgs>&>) and a
 * helper emitMessage() used by subclasses to fire status notifications.
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

} // namespace HydroCouple

#endif // HYDROCOUPLESDK_ABSTRACTEXCHANGEITEM_H
