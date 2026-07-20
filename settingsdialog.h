#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QSettings>

QT_BEGIN_NAMESPACE
namespace Ui { class SettingsDialog; }
QT_END_NAMESPACE

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

private slots:
    // 切换到“通用”选项
    void on_generalBtn_clicked();
    // 切换到“关于”选项
    void on_aboutBtn_clicked();
    // 保存“强制操作”勾选状态
    void on_forceOperationCheck_clicked(bool checked);

private:
    Ui::SettingsDialog *ui;
    QSettings *m_settings;

    // 设置按钮选中样式
    void setButtonSelected(QPushButton *selectedBtn);
};

#endif // SETTINGSDIALOG_H
