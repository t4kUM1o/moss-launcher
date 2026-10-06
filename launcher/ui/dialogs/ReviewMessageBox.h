#pragma once

#include <QDialog>
#include <QTreeWidgetItem>

namespace Ui {
class ReviewMessageBox;
}

class ReviewMessageBox : public QDialog {
    Q_OBJECT

   public:
    static auto create(QWidget* parent, QString&& title, QString&& icon = "") -> ReviewMessageBox*;

    using ResourceInformation = struct res_info {
        QString name;
        QString filename;
        QString provider;
        QStringList required_by;
        QString version_type;
        bool enabled = true;
    };

    void appendResource(ResourceInformation&& info);
    auto deselectedResources() -> QStringList;

    void retranslateUi(QString resources_name);
    // Moss: explicit consent for automatically discovered required MODs.
    void configureModDependencies(const QStringList& dependencyNames, bool checkEnabled, bool lookupComplete = true);
    QStringList missingRequiredMods() const;

    ~ReviewMessageBox() override;

   protected slots:
    void on_toggleDepsButton_clicked();
    void confirmSelection();

   protected:
    ReviewMessageBox(QWidget* parent, const QString& title, const QString& icon);

    Ui::ReviewMessageBox* ui;

    QList<QTreeWidgetItem*> m_deps;
    bool m_deps_checked = true;
};
