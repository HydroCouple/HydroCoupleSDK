#ifndef IDBASEDOUTPUTS_H
#define IDBASEDOUTPUTS_H

#include "abstractoutput.h"
#include "idbasedcomponentdataitem.h"

class HYDROCOUPLESDK_EXPORT IdBasedOutputDouble : public AbstractOutput,
    public IdBasedComponentDataItem<double>,
    public virtual HydroCouple::IIdBasedComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem)
    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)

    using IComponentDataItem::getValue;
    using IComponentDataItem::setValue;

  public:

    IdBasedOutputDouble(const QString& id,
                        const QStringList& identifiers,
                        Dimension *identifierDimension,
                        ValueDefinition* valueDefinition,
                        AbstractModelComponent* modelComponent);

    virtual ~IdBasedOutputDouble() override;

    QStringList identifiers() const override;

    HydroCouple::IDimension* identifierDimension() const override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes, void *data) const override;

    void getValue(int idIndex, void* data) const override;

    void getValues(int idIndex, int stride, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int idIndex, const void * data) override;

    void setValues(int idIndex, int stride, const void *data) override;

    void readData(QXmlStreamReader &reader) override;

    void writeData(QXmlStreamWriter &xmlWriter) const override;

  private:

    Dimension* m_identifierDimension;

};

#endif // IDBASEDOUTPUTS_H
