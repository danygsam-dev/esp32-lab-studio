#include "MainWindow.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QDesktopServices>
#include <QUrl>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QTabWidget>
#include <QGroupBox>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>
#include <QTextCursor>
#include <QAbstractItemView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), process_(new QProcess(this)) {
    setWindowTitle("ESP32 Lab Studio");
    resize(1100, 760);
    buildUi();

    connect(process_, &QProcess::readyReadStandardOutput, this, [this]() {
        appendLog(QString::fromLocal8Bit(process_->readAllStandardOutput()));
    });
    connect(process_, &QProcess::readyReadStandardError, this, [this]() {
        appendLog(QString::fromLocal8Bit(process_->readAllStandardError()));
    });
    connect(process_, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
        status_->setText(QString("Process finished (exit code %1)").arg(code));
        runButton_->setEnabled(true);
        stopButton_->setEnabled(false);
    });

    loadProject();
}

MainWindow::~MainWindow() {
    if (process_->state() != QProcess::NotRunning) {
        process_->kill();
        process_->waitForFinished(1500);
    }
}

QString MainWindow::projectRoot() const {
    // The app is normally launched from the extracted project root.
    return QDir::currentPath();
}

QString MainWindow::mergedFirmwarePath() const {
    return QDir(projectRoot()).filePath("build/firmware/merged_flash.bin");
}

QString MainWindow::selectedSketchDirectory() const {
    if (sketchPath_->text().trimmed().isEmpty()) return {};
    return QFileInfo(sketchPath_->text()).absolutePath();
}

void MainWindow::buildUi() {
    auto *central = new QWidget(this);
    auto *outer = new QVBoxLayout(central);

    auto *header = new QHBoxLayout();
    auto *heading = new QLabel("<h2>ESP32 Lab Studio</h2><span>ESP32-S3 firmware emulator workspace</span>");
    heading->setTextFormat(Qt::RichText);
    header->addWidget(heading);
    header->addStretch();
    status_ = new QLabel("Ready");
    status_->setStyleSheet("color:#16834a;font-weight:600;");
    header->addWidget(status_);
    outer->addLayout(header);

    auto *tabs = new QTabWidget();
    outer->addWidget(tabs, 1);

    // Firmware tab
    auto *firmwareTab = new QWidget();
    auto *fwLayout = new QVBoxLayout(firmwareTab);
    auto *fwGroup = new QGroupBox("Firmware");
    auto *fwForm = new QFormLayout(fwGroup);

    auto *sketchRow = new QWidget();
    auto *sketchLayout = new QHBoxLayout(sketchRow);
    sketchLayout->setContentsMargins(0,0,0,0);
    sketchPath_ = new QLineEdit(QDir(projectRoot()).filePath("examples/BellDemo/BellDemo.ino"));
    auto *browse = new QPushButton("Browse .ino…");
    sketchLayout->addWidget(sketchPath_, 1);
    sketchLayout->addWidget(browse);
    fwForm->addRow("Arduino sketch:", sketchRow);

    ssid_ = new QLineEdit("BellLab");
    password_ = new QLineEdit("belllab123");
    password_->setEchoMode(QLineEdit::Password);
    fwForm->addRow("Emulated Wi-Fi SSID:", ssid_);
    fwForm->addRow("Emulated Wi-Fi password:", password_);
    fwLayout->addWidget(fwGroup);

    auto *buttons = new QHBoxLayout();
    compileButton_ = new QPushButton("Compile .ino → firmware");
    runButton_ = new QPushButton("Run ESP32 emulator");
    stopButton_ = new QPushButton("Stop");
    stopButton_->setEnabled(false);
    auto *dashboard = new QPushButton("Open GPIO dashboard");
    buttons->addWidget(compileButton_);
    buttons->addWidget(runButton_);
    buttons->addWidget(stopButton_);
    buttons->addStretch();
    buttons->addWidget(dashboard);
    fwLayout->addLayout(buttons);

    console_ = new QPlainTextEdit();
    console_->setReadOnly(true);
    console_->setPlaceholderText("Compiler and emulator output will appear here…");
    console_->setStyleSheet("QPlainTextEdit{background:#111827;color:#d1fae5;font-family:monospace;}");
    fwLayout->addWidget(console_, 1);

    auto *note = new QLabel(
        "Firmware execution uses Espressif ESP-EMU. The GPIO dashboard is the live pin-level view; "
        "the component table is a saved wiring plan, not yet an electrical co-simulation.");
    note->setWordWrap(true);
    note->setStyleSheet("color:#9a6700;background:#fff8e1;padding:8px;border-radius:5px;");
    fwLayout->addWidget(note);

    tabs->addTab(firmwareTab, "Firmware & Runtime");

    // Components tab
    auto *componentsTab = new QWidget();
    auto *componentsLayout = new QVBoxLayout(componentsTab);
    auto *componentGroup = new QGroupBox("Component library");
    auto *componentForm = new QFormLayout(componentGroup);

    componentType_ = new QComboBox();
    componentType_->addItems({"LED", "Relay module", "Push button", "Toggle switch", "Buzzer", "Digital sensor", "Clock / RTC"});
    componentName_ = new QLineEdit();
    componentName_->setPlaceholderText("e.g. Main bell relay");
    componentPin_ = new QComboBox();
    componentPin_->addItems({"GPIO0","GPIO1","GPIO2","GPIO3","GPIO4","GPIO5","GPIO6","GPIO7","GPIO8","GPIO9",
                             "GPIO10","GPIO11","GPIO12","GPIO13","GPIO14","GPIO15","GPIO16","GPIO17","GPIO18",
                             "GPIO19","GPIO20","GPIO21","GPIO35","GPIO36","GPIO37","GPIO38","GPIO39","GPIO40",
                             "GPIO41","GPIO42","GPIO43","GPIO44","GPIO45","GPIO46","GPIO47","GPIO48"});
    componentPolarity_ = new QComboBox();
    componentPolarity_->addItems({"Active HIGH / normal", "Active LOW"});
    componentForm->addRow("Component:", componentType_);
    componentForm->addRow("Label:", componentName_);
    componentForm->addRow("ESP32-S3 GPIO:", componentPin_);
    componentForm->addRow("Polarity:", componentPolarity_);
    componentsLayout->addWidget(componentGroup);

    auto *componentButtons = new QHBoxLayout();
    auto *add = new QPushButton("Add component");
    auto *remove = new QPushButton("Remove selected");
    auto *open = new QPushButton("Open project");
    auto *save = new QPushButton("Save project");
    componentButtons->addWidget(add);
    componentButtons->addWidget(remove);
    componentButtons->addStretch();
    componentButtons->addWidget(open);
    componentButtons->addWidget(save);
    componentsLayout->addLayout(componentButtons);

    componentsTable_ = new QTableWidget(0, 4);
    componentsTable_->setHorizontalHeaderLabels({"Component", "Name", "GPIO", "Polarity"});
    componentsTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    componentsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    componentsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    componentsLayout->addWidget(componentsTable_, 1);

    auto *componentNote = new QLabel(
        "Pin mapping notes: this workspace records intended connections. ESP-EMU currently executes "
        "the MCU firmware and exposes GPIO input toggling/output observation in its dashboard. "
        "Relay contacts, LED current, switch bounce, analog circuits and external electrical effects "
        "are not automatically coupled to arbitrary firmware by this project.");
    componentNote->setWordWrap(true);
    componentNote->setStyleSheet("color:#555;padding:8px;");
    componentsLayout->addWidget(componentNote);
    tabs->addTab(componentsTab, "Components & Wiring");

    outer->addWidget(new QLabel("Target: ESP32-S3 • Board profile: generic ESP32-S3 Dev Module • Firmware: Arduino .ino"));
    setCentralWidget(central);

    connect(browse, &QPushButton::clicked, this, &MainWindow::chooseSketch);
    connect(compileButton_, &QPushButton::clicked, this, &MainWindow::compileFirmware);
    connect(runButton_, &QPushButton::clicked, this, &MainWindow::runEmulator);
    connect(stopButton_, &QPushButton::clicked, this, &MainWindow::stopEmulator);
    connect(add, &QPushButton::clicked, this, &MainWindow::addComponent);
    connect(remove, &QPushButton::clicked, this, &MainWindow::removeComponent);
    connect(save, &QPushButton::clicked, this, &MainWindow::saveProject);
    connect(open, &QPushButton::clicked, this, &MainWindow::openProject);
    connect(dashboard, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://espressif.github.io/esp-emulator/"));
    });
}

void MainWindow::appendLog(const QString &text) {
    if (!text.isEmpty()) console_->moveCursor(QTextCursor::End), console_->insertPlainText(text), console_->moveCursor(QTextCursor::End);
}

void MainWindow::chooseSketch() {
    const QString path = QFileDialog::getOpenFileName(this, "Select Arduino sketch", projectRoot(), "Arduino sketches (*.ino)");
    if (!path.isEmpty()) sketchPath_->setText(path);
}

void MainWindow::compileFirmware() {
    if (process_->state() != QProcess::NotRunning) {
        QMessageBox::warning(this, "Busy", "Stop the current process first.");
        return;
    }
    const QString script = QDir(projectRoot()).filePath("scripts/build_firmware.sh");
    if (!QFileInfo::exists(script)) {
        QMessageBox::critical(this, "Project path", "Could not find scripts/build_firmware.sh. Launch the app from the extracted project root.");
        return;
    }
    if (sketchPath_->text().trimmed().isEmpty() || !QFileInfo::exists(sketchPath_->text())) {
        QMessageBox::warning(this, "Sketch required", "Choose a valid .ino file first.");
        return;
    }

    console_->clear();
    appendLog("[Studio] Starting Arduino compilation and flash-image merge…\n");
    process_->setWorkingDirectory(projectRoot());
    process_->setProgram("bash");
    process_->setArguments({script, QFileInfo(sketchPath_->text()).absoluteFilePath()});
    process_->start();
    status_->setText("Compiling firmware…");
    compileButton_->setEnabled(false);
    runButton_->setEnabled(false);
    connect(process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus) {
                compileButton_->setEnabled(true);
                runButton_->setEnabled(true);
                if (code == 0) status_->setText("Firmware compiled");
                else status_->setText("Compilation failed");
            }, Qt::SingleShotConnection);
}

void MainWindow::runEmulator() {
    if (process_->state() != QProcess::NotRunning) {
        QMessageBox::warning(this, "Busy", "A process is already running.");
        return;
    }
    const QString firmware = mergedFirmwarePath();
    if (!QFileInfo::exists(firmware)) {
        QMessageBox::information(this, "Firmware not found", "Compile the sketch first. Expected:\n" + firmware);
        return;
    }

    console_->clear();
    appendLog("[Studio] Launching ESP-EMU for ESP32-S3…\n");
    QString program = QStandardPaths::findExecutable("esp-emu");
    if (program.isEmpty()) {
        QMessageBox::critical(this, "esp-emu not found",
                              "Install Espressif ESP-EMU and ensure esp-emu is on PATH.\nSee README.md.");
        return;
    }

    process_->setWorkingDirectory(projectRoot());
    process_->setProgram(program);
    process_->setArguments({"--chip", "esp32s3", "--firmware", firmware,
                            "--wifi-ssid", ssid_->text(),
                            "--wifi-password", password_->text(),
                            "--net", "user"});
    process_->start();
    status_->setText("ESP-EMU running");
    runButton_->setEnabled(false);
    stopButton_->setEnabled(true);
}

void MainWindow::stopEmulator() {
    if (process_->state() != QProcess::NotRunning) {
        process_->terminate();
        if (!process_->waitForFinished(2000)) process_->kill();
    }
    runButton_->setEnabled(true);
    stopButton_->setEnabled(false);
    status_->setText("Stopped");
}

void MainWindow::addComponent() {
    const int row = componentsTable_->rowCount();
    componentsTable_->insertRow(row);
    const QString name = componentName_->text().trimmed().isEmpty()
        ? componentType_->currentText() + " " + QString::number(row + 1)
        : componentName_->text().trimmed();
    const QStringList values = {componentType_->currentText(), name, componentPin_->currentText(), componentPolarity_->currentText()};
    for (int col = 0; col < values.size(); ++col)
        componentsTable_->setItem(row, col, new QTableWidgetItem(values[col]));
    componentName_->clear();
}

void MainWindow::removeComponent() {
    const int row = componentsTable_->currentRow();
    if (row >= 0) componentsTable_->removeRow(row);
}

void MainWindow::saveProject() {
    QJsonArray array;
    for (int row = 0; row < componentsTable_->rowCount(); ++row) {
        QJsonObject item;
        item["type"] = componentsTable_->item(row, 0)->text();
        item["name"] = componentsTable_->item(row, 1)->text();
        item["gpio"] = componentsTable_->item(row, 2)->text();
        item["polarity"] = componentsTable_->item(row, 3)->text();
        array.append(item);
    }
    QJsonObject root;
    root["schema_version"] = 1;
    root["chip"] = "esp32s3";
    root["board"] = "ESP32-S3 Dev Module";
    root["sketch"] = sketchPath_->text();
    root["wifi_ssid"] = ssid_->text();
    root["components"] = array;

    const QString path = QFileDialog::getSaveFileName(this, "Save simulation project", projectRoot() + "/bell-lab.json", "ESP32 Lab project (*.json)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Save failed", file.errorString());
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    currentProjectFile_ = path;
    status_->setText("Project saved");
}

void MainWindow::loadProject() {
    loadProjectFrom(QDir(projectRoot()).filePath("bell-lab.json"));
}

void MainWindow::openProject() {
    const QString path = QFileDialog::getOpenFileName(this, "Open simulation project", projectRoot(), "ESP32 Lab project (*.json)");
    if (!path.isEmpty()) loadProjectFrom(path);
}

void MainWindow::loadProjectFrom(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (path != QDir(projectRoot()).filePath("bell-lab.json"))
            QMessageBox::warning(this, "Open failed", file.errorString());
        return;
    }
    const auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        QMessageBox::warning(this, "Invalid project", "The selected file is not a valid ESP32 Lab JSON project.");
        return;
    }
    const auto root = doc.object();
    sketchPath_->setText(root.value("sketch").toString(sketchPath_->text()));
    ssid_->setText(root.value("wifi_ssid").toString(ssid_->text()));
    componentsTable_->setRowCount(0);
    const auto components = root.value("components").toArray();
    for (const auto &value : components) {
        const auto item = value.toObject();
        const int row = componentsTable_->rowCount();
        componentsTable_->insertRow(row);
        const QStringList values = {item.value("type").toString(), item.value("name").toString(),
                                    item.value("gpio").toString(), item.value("polarity").toString()};
        for (int col = 0; col < values.size(); ++col)
            componentsTable_->setItem(row, col, new QTableWidgetItem(values[col]));
    }
    currentProjectFile_ = path;
    status_->setText("Project loaded");
}
