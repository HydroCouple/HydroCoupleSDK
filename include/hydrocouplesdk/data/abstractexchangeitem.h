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
 * Component authors derive from AbstractInput or AbstractOutput (or use the
 * typed items built on them), never from this class directly: the standard
 * says an exchange item is always one or the other.
 *
 * \threadsafety Signal connections are thread-safe (std::shared_mutex);
 * data mutation is not.
 */
class HYDROCOUPLESDK_EXPORT AbstractExchangeItem
    : public AbstractComponentDataItem,
      public virtual HydroCouple::IExchangeItem
{
public:
    /*!
     * \brief Construct with identifier, dimensions, value definition and
     *        owning component.
     * \param id              Identifier, unique within the owning component;
     *                        empty generates a UUID.
     * \param dimensions      Axes in storage order (borrowed; the list is copied).
     * \param valueDefinition Value definition (borrowed; may be nullptr).
     * \param modelComponent  Owning component (borrowed; may be nullptr).
     */
    AbstractExchangeItem(std::string_view id,
                          const std::vector<Dimension *> &dimensions,
                          ValueDefinition *valueDefinition,
                          HydroCouple::IModelComponent *modelComponent);

    /*!
     * \brief Construct with identifier, caption, dimensions, value definition
     *        and owning component.
     * \param id              Identifier, unique within the owning component;
     *                        empty generates a UUID.
     * \param caption         Human-readable caption.
     * \param dimensions      Axes in storage order (borrowed; the list is copied).
     * \param valueDefinition Value definition (borrowed; may be nullptr).
     * \param modelComponent  Owning component (borrowed; may be nullptr).
     */
    AbstractExchangeItem(std::string_view id,
                          std::string_view caption,
                          const std::vector<Dimension *> &dimensions,
                          ValueDefinition *valueDefinition,
                          HydroCouple::IModelComponent *modelComponent);

    //! Destroys the item; borrowed dimensions and value definition are not touched.
    ~AbstractExchangeItem() override = default;

protected:
    /*!
     * \brief Fires the itemChanged signal with a status message.
     * \details What the implementation does: it fires the value-changed
     *          signal with an empty start and count (meaning "the whole item
     *          may have changed") and discards \p message; no slot receives
     *          the text.
     * \param message  Human-readable description of the change.
     */
    void emitMessage(const std::string &message);
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTEXCHANGEITEM_H
