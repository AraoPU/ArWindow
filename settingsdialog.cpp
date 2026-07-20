#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QStyle>
#include <QLabel>
#include <QPushButton>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
    , m_settings(new QSettings("ArWindow", "Settings", this))
{
    ui->setupUi(this);
    this->setWindowTitle("设置");
    // 固定大小后启用最大化按钮没意义，会显示但点不动（Bug 4）
    this->setFixedSize(500, 400);

    // 美化设置窗口样式表
    this->setStyleSheet(R"(
        QDialog {
            background-color: #f5f5f5;
        }
        /* 左侧侧边栏容器 */
        QWidget#sidebarWidget {
            background-color: #2c3e50;
            border-radius: 4px 0 0 4px;
        }
        /* 左侧按钮通用样式 */
        QPushButton#generalBtn, QPushButton#aboutBtn {
            background-color: transparent;
            color: #ecf0f1;
            border: none;
            text-align: left;
            padding: 12px 20px;
            font-size: 14px;
            margin: 5px 0;
            border-left: 3px solid transparent;
        }
        /* 按钮悬停样式 */
        QPushButton#generalBtn:hover, QPushButton#aboutBtn:hover {
            background-color: #34495e;
        }
        /* 选中按钮样式（变色+左侧边框） */
        QPushButton[selected="true"] {
            background-color: #34495e;
            border-left: 3px solid #3498db;
            color: white;
            font-weight: bold;
        }
        /* 右侧内容区域 */
        QStackedWidget {
            background-color: white;
            border: 1px solid #e0e0e0;
            border-radius: 0 4px 4px 0;
            padding: 20px;
        }
        /* 勾选框样式 */
        QCheckBox {
            font-size: 14px;
            padding: 8px 0;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
        }
        /* 关于页面文本 */
        QLabel#aboutLabel {
            font-size: 14px;
            color: #2c3e50;
        }
    )");

    // 初始化选中“通用”按钮
    setButtonSelected(ui->generalBtn);
    ui->stackedWidget->setCurrentWidget(ui->generalPage);

    // 读取保存的“强制操作”状态
    bool forceOp = m_settings->value("General/ForceOperation", false).toBool();
    ui->forceOperationCheck->setChecked(forceOp);

    // 填充关于页面信息（Bug 14：动态获取 Qt 版本）
    ui->aboutLabel->setText(
        QString("<h3 style='color:#2c3e50;'>ArWindow 窗口管理器</h3>"
                "<p>版本：1.0.0</p>"
                "<p>基于 Qt %1 开发</p>"
                "<p>功能：强制控制窗口最大化/最小化/关闭</p>"
                "<p>适配平台：Windows</p>"
                "<hr style='border:0; border-top:1px solid #e0e0e0; margin:12px 0;'>"
                "<p style='color:#7f8c8d; font-size:12px;'>"
                "Copyright &copy; 2026 Ar Studio. All Rights Reserved.<br>"
                "本软件基于 <b>MIT License</b> 开源发布<br>"
                "源代码与许可证详见项目根目录 LICENSE 文件"
                "</p>")
            .arg(QLatin1String(qVersion())));
}

SettingsDialog::~SettingsDialog()
{
    // m_settings 以 this 为父对象，Qt 会自动销毁，不能再次 delete（Bug 1）
    delete ui;
}

void SettingsDialog::setButtonSelected(QPushButton *selectedBtn)
{
    // 使用动态属性切换样式（Bug 5：动态修改 objectName 不会触发 QSS 重算）
    ui->generalBtn->setProperty("selected", ui->generalBtn == selectedBtn);
    ui->aboutBtn->setProperty("selected", ui->aboutBtn == selectedBtn);

    // 手动触发样式刷新
    for (QPushButton *btn : { ui->generalBtn, ui->aboutBtn })
    {
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    }
}

void SettingsDialog::on_generalBtn_clicked()
{
    setButtonSelected(ui->generalBtn);
    ui->stackedWidget->setCurrentWidget(ui->generalPage);
}

void SettingsDialog::on_aboutBtn_clicked()
{
    setButtonSelected(ui->aboutBtn);
    ui->stackedWidget->setCurrentWidget(ui->aboutPage);
}

void SettingsDialog::on_forceOperationCheck_clicked(bool checked)
{
    m_settings->setValue("General/ForceOperation", checked);
}
