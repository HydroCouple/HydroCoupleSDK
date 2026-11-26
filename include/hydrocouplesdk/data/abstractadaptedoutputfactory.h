// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   abstractadaptedoutputfactory.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Concrete base classes for IAdaptedOutputFactory and
 *         IAdaptedOutputFactoryComponent.
 */

#ifndef HYDROCOUPLESDK_ABSTRACTADAPTEDOUTPUTFACTORY_H
#define HYDROCOUPLESDK_ABSTRACTADAPTEDOUTPUTFACTORY_H

#include "hydrocouplesdk/core/identity.h"
#include "hydrocouple.h"

namespace HydroCouple::SDK {

class AbstractAdaptedOutputFactoryComponentInfo;

// ── AbstractAdaptedOutputFactory ─────────────────────────────────────────

/*!
 * \brief Concrete base class for HydroCouple::IAdaptedOutputFactory.
 *
 * Subclasses implement getAvailableAdaptedOutputIds() and
 * createAdaptedOutput() to produce transformation chains between
 * an IOutput and an IInput.
 *
 * \details A factory reaches a run in one of two ways: a component info
 * lists it (AbstractModelComponentInfo::addAdaptedOutputFactory()), which
 * makes it available on that component's outputs, or a host installs it as
 * a standalone factory (IO::ModelInitializer::setAdapterFactoryResolver()).
 * The chain builder asks each candidate for its offerings on the current
 * output and creates the step whose offering id the composition names. A
 * composition document can name the factory by its id().
 *
 * The factory must outlive every adapter it creates, because each adapter
 * keeps a pointer to it (IAdaptedOutput::adaptedOutputFactory()).
 *
 * \threadsafety Not thread-safe on its own.
 */
class HYDROCOUPLESDK_EXPORT AbstractAdaptedOutputFactory
    : public Identity,
      public virtual HydroCouple::IAdaptedOutputFactory
{
public:
    /*!
     * \brief Construct with the factory identifier.
     * \param id Factory identifier, which composition documents use to name
     *           the factory of a chain step; empty generates a UUID.
     */
    explicit AbstractAdaptedOutputFactory(std::string_view id);
    //! Destroys the factory; adapters it created must already be gone.
    ~AbstractAdaptedOutputFactory() override = default;

    //! Not copyable: owns non-copyable state (unique_ptr / mutex /
    //! atomic). Declared rather than left implicit because the class is
    //! exported, and __declspec(dllexport) instantiates every member --
    //! including the implicit copy assignment, which cannot compile
    //! against those members. Other toolchains only instantiate it on use.
    AbstractAdaptedOutputFactory(const AbstractAdaptedOutputFactory &)            = delete;
    AbstractAdaptedOutputFactory &operator=(const AbstractAdaptedOutputFactory &) = delete;

    // IAdaptedOutputFactory — pure virtual, subclasses implement
    /*!
     * \brief The adaptations this factory can make for \p provider.
     * \details Pure virtual. Returns identities that describe offerings (an
     *          id and a caption), not instances: pass one back to
     *          createAdaptedOutput(). The identities are owned by the factory
     *          and stay valid while it lives.
     * \param[in] provider The output to adapt; an implementation returns an
     *                     empty list for nullptr or for an output it cannot
     *                     adapt.
     * \param[in] consumer The input the result will feed, or nullptr to list
     *                     every offering for the provider.
     * \return The offerings, possibly empty.
     */
    std::vector<HydroCouple::IIdentity *>
    getAvailableAdaptedOutputIds(const HydroCouple::IOutput *provider,
                                  const HydroCouple::IInput  *consumer = nullptr) override = 0;

    /*!
     * \brief Creates the adapter an offering describes, over \p provider.
     * \details Pure virtual. The interface contract: the returned adapter is
     *          already registered with \p provider (IOutput::addAdaptedOutput())
     *          and the caller owns it. Its arguments hold their defaults; the
     *          caller sets them and then calls initialize().
     * \param[in] adaptedProviderId One of the identities returned by
     *                              getAvailableAdaptedOutputIds().
     * \param[in] provider          The output (or adapter) to adapt; borrowed.
     * \param[in] consumer          The input the result will feed, or nullptr.
     * \return The new adapter, or nullptr when the id is unknown or the
     *         adaptation is impossible.
     */
    std::unique_ptr<HydroCouple::IAdaptedOutput>
    createAdaptedOutput(HydroCouple::IIdentity *adaptedProviderId,
                         HydroCouple::IOutput   *provider,
                         HydroCouple::IInput    *consumer = nullptr) override = 0;
};


// ── AbstractAdaptedOutputFactoryComponent ────────────────────────────────

/*!
 * \brief Concrete base class for HydroCouple::IAdaptedOutputFactoryComponent.
 *
 * Extends AbstractAdaptedOutputFactory with a back-pointer to its
 * IAdaptedOutputFactoryComponentInfo.
 *
 * \details A factory component is a factory delivered as a plugin of its own,
 * not through a model component: its info
 * (AbstractAdaptedOutputFactoryComponentInfo) creates instances, and a host
 * offers those instances to the chain builder as standalone factories.
 * Subclasses still implement getAvailableAdaptedOutputIds() and
 * createAdaptedOutput(). The info is borrowed and must outlive the instance.
 */
class HYDROCOUPLESDK_EXPORT AbstractAdaptedOutputFactoryComponent
    : public AbstractAdaptedOutputFactory,
      public virtual HydroCouple::IAdaptedOutputFactoryComponent
{
public:
    /*!
     * \brief Construct with an identifier and the info that describes it.
     * \param id   Factory identifier; empty generates a UUID.
     * \param info The component info this instance came from (borrowed; may
     *             be nullptr until setComponentInfo() is called).
     */
    AbstractAdaptedOutputFactoryComponent(std::string_view id,
                                           AbstractAdaptedOutputFactoryComponentInfo *info);
    //! Destroys the factory component; the info is not touched.
    ~AbstractAdaptedOutputFactoryComponent() override = default;

    /*!
     * \brief The metadata of this factory component.
     * \return The info given at construction or by setComponentInfo()
     *         (non-owning; may be nullptr).
     */
    [[nodiscard]] HydroCouple::IAdaptedOutputFactoryComponentInfo *componentInfo() const override;

protected:
    /*!
     * \brief Replaces the info pointer.
     * \param[in] info The component info (borrowed; may be nullptr).
     */
    void setComponentInfo(AbstractAdaptedOutputFactoryComponentInfo *info);

private:
    AbstractAdaptedOutputFactoryComponentInfo *m_componentInfo; //!< Describing info (borrowed).
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTADAPTEDOUTPUTFACTORY_H
