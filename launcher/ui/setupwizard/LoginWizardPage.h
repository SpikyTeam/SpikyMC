#pragma once
#include <QWidget>
#include "BaseWizardPage.h"
#include "minecraft/auth/MinecraftAccount.h"

namespace Ui {
class LoginWizardPage;
}

class LoginWizardPage : public BaseWizardPage {
    Q_OBJECT

   public:
    explicit LoginWizardPage(QWidget* parent = nullptr);
    ~LoginWizardPage();

    void initializePage() override;
    bool validatePage() override;
    void retranslate() override;
   private slots:
    void on_pushButton_clicked();
    void on_offlineButton_clicked();

   private:
    void storeAccount(const MinecraftAccountPtr& account);
    Ui::LoginWizardPage* ui;
};
