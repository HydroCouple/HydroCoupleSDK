/*!
 * \file componentinfo.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version   1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU General Public License as published by the Free Software Foundation;
 * either version 3 of the License, or (at your option) any later version.
 * This file and its associated files is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 * \copyright Copyright 2014-2018, Caleb Buahin, All rights reserved.
 * \date 2014-2018
 * \pre
 * \bug
 * \warning
 * \todo
 */

#ifndef COMPONENTINFO_H
#define COMPONENTINFO_H

#include "identity.h"

/*!
 * \brief Implementation of the IComponentInfo. This is the core interface specifying meta data about a component.
 *
 * \details This interface must not be implemented directly. It must be inherited
 */
class HYDROCOUPLESDK_EXPORT  ComponentInfo : public Identity,
    public virtual HydroCouple::IComponentInfo
{
    Q_OBJECT

    Q_INTERFACES(HydroCouple::IComponentInfo)

    Q_PROPERTY(QString LibraryPath READ libraryFilePath NOTIFY propertyChanged)
    Q_PROPERTY(QString IconFilePath READ iconFilePath NOTIFY propertyChanged)
    Q_PROPERTY(QString Vendor READ vendor NOTIFY propertyChanged)
    Q_PROPERTY(QStringList Documentation READ documentation NOTIFY propertyChanged)
    Q_PROPERTY(QString License READ license WRITE setLicense NOTIFY propertyChanged)
    Q_PROPERTY(QString Copyright READ copyright NOTIFY propertyChanged)
    Q_PROPERTY(QString Url READ url NOTIFY propertyChanged)
    Q_PROPERTY(QString Email READ email NOTIFY propertyChanged)
    Q_PROPERTY(QString Version READ version NOTIFY propertyChanged)
    Q_PROPERTY(QString Category READ category NOTIFY propertyChanged)

  public:

    /*!
       * \brief ComponentInfo
       * \param parent
       */
    ComponentInfo(QObject* parent = nullptr);

    /*!
       * \brief ~ComponentInfo
       */
    virtual ~ComponentInfo(){}

    /*!
       * \brief File path to Component library.
       * \returns Path to the library location from which this component was created.
       */
    virtual QString libraryFilePath() const override;

    /*!
       * \brief Sets file path to Component library.
       * \param FilePath to the libary from which this component was created.
       */
    virtual void setLibraryFilePath(const QString &filePath) override;

    /*!
       * \brief File path to Component icon.
       * Must be specified relative to the component library.
       *
       * \returns filePath to icon for component.
       */
    virtual QString iconFilePath() const override;

    /*!
       * \brief Component developer information.
       * \returns Name of vendor the developed this component.
       */
    virtual QString vendor() const override;

    /*!
       * \brief Publications associated with this component.
       * \returns Citations of publications related to this component.
       */
    virtual QStringList documentation() const override;

    /*!
       * \brief Component license info.
       * \returns QString representing the license information. HTML tags can be added to it.
       */
    virtual QString license() const override;


    void setLicense(const QString &license);


    /*!
       * \brief Component copyright info.
       * \returns QString representing the copyright information associated with this component.
       */
    virtual QString copyright() const override;

    /*!
       * \brief Component developer url.
       * \returns QString representing the url for the developer.
       */
    virtual QString url() const override;

    /*!
       * \brief Component developer email.
       * \returns email as QString.
       */
    virtual QString email() const override;

    /*!
       * \brief Component version info.
       * \returns QString representing the version of this component.
       */
    virtual QString version() const override;

    /*!
       * \brief Component category.
       * \returns the category that this component belongs to.
       * Sub-categories can be specified by separating categories using the backslash
       * e.g., Hydrology\\Snowmelt.
       */
    virtual QString category() const override;


    bool hasValidLicense() const override;


    virtual bool validateLicense(const QString& licenseInfo, QString &validationMessage) override;


    virtual bool validateLicense(QString &validationMessage) override;

  signals:

    /*!
       * \brief Called to emit signal/event when property of child class changes.
       * \param propertyName is a string representing the name of the property
       * \param value is a QVariant representing the value of the property
       */
    void propertyChanged(const QString &propertyName) override;


  protected:

    void setIconFilePath(const QString &filepath);

    void setVendor(const QString &vendor);

    void setDocumentation(const QStringList &documentation);

    void addDocumentation(const QString &document);

    void setCopyright(const QString &copyright);

    void setUrl(const QString &url);

    void setEmail(const QString &email);

    void setVersion(const QString &version);

    void setCategory(const QString &category);

  private:

    QString m_iconFilePath, m_vendor,
    m_license, m_copyright, m_url,
    m_email, m_version, m_category,
    m_filePath;
    QList<QString> m_documentation;
};

Q_DECLARE_METATYPE(ComponentInfo*)

#endif // COMPONENTINFO_H
