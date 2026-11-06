#pragma once

#include <QMainWindow>
#include <QProcess>
#include <QJsonArray>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void buildUi();
    void appendLog(const QString &text);
    void chooseSketch();
    void compileFirmware();
    void runEmulator();
    void stopEmulator();
    void addComponent();
    void removeComponent();
    void saveProject();
    void loadProject();
    void openProject();
    void loadProjectFrom(const QString &path);
    QString projectRoot() const;
    QString mergedFirmwarePath() const;
    QString selectedSketchDirectory() const;

    QProcess *process_;
    QLineEdit *sketchPath_;
    QLineEdit *ssid_;
    QLineEdit *password_;
    QLineEdit *componentName_;
    QComboBox *componentType_;
    QComboBox *componentPin_;
    QComboBox *componentPolarity_;
    QTableWidget *componentsTable_;
    QPlainTextEdit *console_;
    QLabel *status_;
    QPushButton *compileButton_;
    QPushButton *runButton_;
    QPushButton *stopButton_;
    QString currentProjectFile_;
};
