#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QTimer>
#include <QStyle>
#include <QPixmap>
#include <QImage>
#include <QGuiApplication>
#include <QCursor>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_settingsDialog(new SettingsDialog(this))
    , m_settings(new QSettings("ArWindow", "Settings", this))
{
    ui->setupUi(this);
    this->setWindowTitle("ArWindow - 窗口管理器");

    // 记录自身进程ID，用于过滤自身窗口（Bug 8）
#ifdef Q_OS_WIN
    m_selfPid = static_cast<quint32>(GetCurrentProcessId());
#endif

    // 保留默认窗口标志，仅启用常用按钮（Bug 16：避免在 setupUi 后用 Qt::Window 覆盖标志导致窗口重建）
    this->setWindowFlags(this->windowFlags()
                         | Qt::WindowMinimizeButtonHint
                         | Qt::WindowMaximizeButtonHint
                         | Qt::WindowCloseButtonHint);
    this->setMinimumSize(800, 600);

    // 初始化树形列表
    ui->windowTreeWidget->setColumnCount(4);
    ui->windowTreeWidget->setHeaderLabels({"图标", "窗口标题", "进程ID", "窗口句柄"});
    ui->windowTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->windowTreeWidget->setColumnWidth(0, 40);
    ui->windowTreeWidget->setColumnWidth(1, 350);
    ui->windowTreeWidget->setColumnWidth(2, 80);
    ui->windowTreeWidget->setColumnWidth(3, 100);
    ui->windowTreeWidget->setSortingEnabled(false);

    // UI美化样式表
    this->setStyleSheet(R"(
        QMainWindow {
            background-color: #f5f5f5;
        }
        QTreeWidget {
            background-color: white;
            border: 1px solid #e0e0e0;
            border-radius: 4px;
            padding: 5px;
        }
        QTreeWidget::header {
            background-color: #e9e9e9;
            font-weight: bold;
        }
        QTreeWidget::item:selected {
            background-color: #2196F3;
            color: white;
        }
        QPushButton {
            background-color: #2196F3;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: #1976D2;
        }
        QPushButton:pressed {
            background-color: #0d47a1;
        }
        QPushButton#settingsBtn {
            background-color: #4CAF50;
        }
        QPushButton#settingsBtn:hover {
            background-color: #388E3C;
        }
        QPushButton#closeWindowBtn {
            background-color: #F44336;
        }
        QPushButton#closeWindowBtn:hover {
            background-color: #D32F2F;
        }
        QPushButton#restoreWindowBtn {
            background-color: #FF9800;
        }
        QPushButton#restoreWindowBtn:hover {
            background-color: #F57C00;
        }
        QPushButton#refreshListBtn {
            background-color: #9C27B0;
        }
        QPushButton#refreshListBtn:hover {
            background-color: #7B1FA2;
        }
    )");

    // 首次启动时刷新列表
    refreshWindowList();
}

MainWindow::~MainWindow()
{
    // m_settings 以 this 为父对象，Qt 会自动销毁，不能再次 delete（Bug 1）
    delete ui;
}

void MainWindow::addWindowItem(const QIcon &icon, const QString &title, quint32 pid, quintptr hWnd)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(ui->windowTreeWidget);
    item->setIcon(0, icon);
    item->setText(1, title);
    item->setText(2, QString::number(pid));
    item->setText(3, QString::number(hWnd, 16));
    item->setData(3, Qt::UserRole, QVariant::fromValue(hWnd));
}

void MainWindow::refreshWindowList()
{
    // Bug 15：刷新期间显示等待光标并禁用列表交互，提供视觉反馈
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    ui->windowTreeWidget->setEnabled(false);

    ui->windowTreeWidget->clear();
#ifdef Q_OS_WIN
    enumerateWindows();
#else
    QTreeWidgetItem *item = new QTreeWidgetItem(ui->windowTreeWidget);
    item->setIcon(0, style()->standardIcon(QStyle::SP_MessageBoxWarning));
    item->setText(1, "仅支持Windows平台");
    item->setText(2, "-");
    item->setText(3, "-");
#endif

    ui->windowTreeWidget->setEnabled(true);
    QGuiApplication::restoreOverrideCursor();
}

#ifdef Q_OS_WIN
// 将局部静态函数封装，避免与 MainWindow:: 同名冲突
static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    MainWindow *window = reinterpret_cast<MainWindow*>(lParam);
    if (!IsWindowVisible(hWnd))
        return TRUE;

    // 过滤自身窗口（Bug 8）
    DWORD pid = 0;
    GetWindowThreadProcessId(hWnd, &pid);
    if (pid == static_cast<DWORD>(window->selfPid()))
        return TRUE;

    wchar_t title[256] = {0};
    GetWindowTextW(hWnd, title, 256);
    QString windowTitle = QString::fromWCharArray(title).trimmed();
    if (windowTitle.isEmpty())
        return TRUE;

    QIcon icon = window->getWindowIcon(reinterpret_cast<quintptr>(hWnd), pid);
    window->addWindowItem(icon, windowTitle, pid, reinterpret_cast<quintptr>(hWnd));
    return TRUE;
}

void MainWindow::enumerateWindows()
{
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(this));
}

QIcon MainWindow::getWindowIcon(quintptr hWndU, quint32 pidU)
{
    HWND hWnd = reinterpret_cast<HWND>(hWndU);
    QIcon defaultIcon = style()->standardIcon(QStyle::SP_ComputerIcon);
    HICON hIcon = nullptr;

    // 标记图标来源：只有 SHGetFileInfo 返回的图标需要调用者销毁（Bug 2）
    bool iconOwnedByCaller = false;

    // 使用 SendMessageTimeoutW 替代 SendMessage（Bug 15：避免目标窗口无响应时拖死本程序 UI）
    DWORD_PTR msgResult = 0;
    if (SendMessageTimeoutW(hWnd, WM_GETICON, ICON_BIG, 0,
                            SMTO_BLOCK | SMTO_ABORTIFHUNG, 200, &msgResult))
    {
        hIcon = reinterpret_cast<HICON>(msgResult);
    }
    if (!hIcon)
        hIcon = reinterpret_cast<HICON>(GetClassLongPtr(hWnd, GCLP_HICON));

    if (!hIcon && pidU != 0)
    {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pidU);
        if (hProcess)
        {
            wchar_t exePath[MAX_PATH] = {0};
            if (GetModuleFileNameExW(hProcess, nullptr, exePath, MAX_PATH))
            {
                SHFILEINFOW sfi;
                memset(&sfi, 0, sizeof(SHFILEINFOW));
                SHGetFileInfoW(exePath, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON);
                hIcon = sfi.hIcon;
                iconOwnedByCaller = (hIcon != nullptr);
            }
            CloseHandle(hProcess);
        }
    }

    if (hIcon)
    {
        QIcon icon;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        icon = QIcon(QPixmap::fromImage(QImage::fromHICON(hIcon)));
#else
        icon = QIcon(QPixmap::fromWinHICON(hIcon));
#endif
        // 仅销毁调用者拥有的图标（Bug 2）
        if (iconOwnedByCaller)
            DestroyIcon(hIcon);
        return icon;
    }

    return defaultIcon;
}
#endif

bool MainWindow::performWindowOperation(quintptr hWndU, const QString &operation)
{
    // 操作前校验窗口句柄是否仍然有效（Bug 7）
#ifdef Q_OS_WIN
    HWND hWnd = reinterpret_cast<HWND>(hWndU);
    if (!hWnd || !IsWindow(hWnd))
    {
        QMessageBox::warning(this, "操作失败", "目标窗口已不存在，请刷新列表！");
        return false;
    }
#else
    Q_UNUSED(hWndU)
    Q_UNUSED(operation)
    return false;
#endif

    bool forceMode = isForceOperationEnabled();
    bool result = false;

#ifdef Q_OS_WIN
    LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);

    // 修改样式后必须调用 SetWindowPos 让新样式生效（Bug 6）
    auto applyStyle = [hWnd]() {
        SetWindowPos(hWnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    };

    if (operation == "maximize")
    {
        if (forceMode)
        {
            style &= ~WS_DISABLED;
            style |= WS_MAXIMIZEBOX;
            SetWindowLongPtr(hWnd, GWL_STYLE, style);
            applyStyle();
        }
        if (forceMode || (style & WS_MAXIMIZEBOX))
        {
            result = ShowWindow(hWnd, SW_MAXIMIZE) != FALSE;
        }
        else
        {
            QMessageBox::warning(this, "操作失败", "该窗口禁止最大化，请启用强制操作后重试！");
        }
    }
    else if (operation == "minimize")
    {
        if (forceMode)
        {
            style &= ~WS_DISABLED;
            style |= WS_MINIMIZEBOX;
            SetWindowLongPtr(hWnd, GWL_STYLE, style);
            applyStyle();
        }
        if (forceMode || (style & WS_MINIMIZEBOX))
        {
            result = ShowWindow(hWnd, SW_MINIMIZE) != FALSE;
        }
        else
        {
            QMessageBox::warning(this, "操作失败", "该窗口禁止最小化，请启用强制操作后重试！");
        }
    }
    else if (operation == "restore")
    {
        if (forceMode)
        {
            style &= ~WS_DISABLED;
            style |= WS_MAXIMIZEBOX;
            SetWindowLongPtr(hWnd, GWL_STYLE, style);
            applyStyle();
        }
        result = ShowWindow(hWnd, SW_RESTORE) != FALSE;
    }
    else if (operation == "close")
    {
        if (forceMode)
        {
            // 强制模式：仅发送 WM_CLOSE，给程序留一次保存数据的机会（Bug 3）
            // 不再直接 TerminateProcess 杀进程，避免误杀多窗口应用导致数据丢失
            result = PostMessage(hWnd, WM_CLOSE, 0, 0) != FALSE;
        }
        else
        {
            result = SendMessage(hWnd, WM_CLOSE, 0, 0) == 0;
        }
    }
#endif

    return result;
}

bool MainWindow::isForceOperationEnabled()
{
    return m_settings->value("General/ForceOperation", false).toBool();
}

// 返回自身进程ID（供 EnumWindowsProc 过滤使用）
quint32 MainWindow::selfPid() const
{
    return m_selfPid;
}

void MainWindow::on_forceMaxBtn_clicked()
{
    QTreeWidgetItem *selectedItem = ui->windowTreeWidget->currentItem();
    if (!selectedItem)
    {
        QMessageBox::information(this, "提示", "请先选中一个窗口！");
        return;
    }

    quintptr hWnd = selectedItem->data(3, Qt::UserRole).value<quintptr>();
    if (!performWindowOperation(hWnd, "maximize"))
        QMessageBox::warning(this, "操作失败", "最大化操作未成功！");
}

void MainWindow::on_forceMinBtn_clicked()
{
    QTreeWidgetItem *selectedItem = ui->windowTreeWidget->currentItem();
    if (!selectedItem)
    {
        QMessageBox::information(this, "提示", "请先选中一个窗口！");
        return;
    }

    quintptr hWnd = selectedItem->data(3, Qt::UserRole).value<quintptr>();
    if (!performWindowOperation(hWnd, "minimize"))
        QMessageBox::warning(this, "操作失败", "最小化操作未成功！");
}

void MainWindow::on_restoreWindowBtn_clicked()
{
    QTreeWidgetItem *selectedItem = ui->windowTreeWidget->currentItem();
    if (!selectedItem)
    {
        QMessageBox::information(this, "提示", "请先选中一个窗口！");
        return;
    }

    quintptr hWnd = selectedItem->data(3, Qt::UserRole).value<quintptr>();
    if (!performWindowOperation(hWnd, "restore"))
        QMessageBox::warning(this, "操作失败", "还原操作未成功！");
}

void MainWindow::on_closeWindowBtn_clicked()
{
    QTreeWidgetItem *selectedItem = ui->windowTreeWidget->currentItem();
    if (!selectedItem)
    {
        QMessageBox::information(this, "提示", "请先选中一个窗口！");
        return;
    }

    quintptr hWnd = selectedItem->data(3, Qt::UserRole).value<quintptr>();
    if (QMessageBox::question(this, "确认", "是否确定关闭该窗口？") == QMessageBox::Yes)
    {
        bool forceMode = isForceOperationEnabled();
        // 强制模式下使用二次确认，提醒用户（Bug 3 相关）
        if (forceMode)
        {
            if (QMessageBox::warning(this, "强制关闭确认",
                                     "当前为强制模式，目标程序可能无法保存未保存的数据。\n是否继续？",
                                     QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
                return;
        }
        if (!performWindowOperation(hWnd, "close"))
        {
            QMessageBox::warning(this, "操作失败", "关闭操作未成功！");
            return;
        }
        // 延迟刷新，避免异步 PostMessage(WM_CLOSE) 还未真正销毁窗口（Bug 11）
        QTimer::singleShot(300, this, [this]() { refreshWindowList(); });
    }
}

// 新增：刷新列表按钮点击事件
void MainWindow::on_refreshListBtn_clicked()
{
    refreshWindowList();
}

void MainWindow::on_settingsBtn_clicked()
{
    m_settingsDialog->exec();
}
