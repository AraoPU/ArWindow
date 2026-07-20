#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QSettings>
#include <QIcon>
#include "settingsdialog.h"

// Windows API 相关声明（仅Windows平台）
#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#include <psapi.h> // 新增：GetModuleFileNameExW 需要的头文件
#endif

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 公共接口：添加窗口列表项（含图标）
    // 使用 quint32 / quintptr 以保证非 Windows 平台也可编译
    void addWindowItem(const QIcon &icon, const QString &title, quint32 pid, quintptr hWnd);
    // 公共成员函数：获取窗口图标
    QIcon getWindowIcon(quintptr hWnd, quint32 pid);
    // 返回自身进程ID（供 EnumWindowsProc 过滤使用）
    quint32 selfPid() const;

private slots:
    void refreshWindowList(); // 刷新列表（手动触发）
    void on_forceMaxBtn_clicked();
    void on_forceMinBtn_clicked();
    void on_closeWindowBtn_clicked();
    void on_settingsBtn_clicked();
    void on_restoreWindowBtn_clicked();
    // 新增：刷新列表按钮的槽函数
    void on_refreshListBtn_clicked();

private:
    Ui::MainWindow *ui;
    SettingsDialog *m_settingsDialog;
    QSettings *m_settings;

    void enumerateWindows();
    bool performWindowOperation(quintptr hWnd, const QString &operation);
    bool isForceOperationEnabled();
    // 应用自身进程ID（用于过滤自己）
    quint32 m_selfPid = 0;
};

#endif // MAINWINDOW_H
