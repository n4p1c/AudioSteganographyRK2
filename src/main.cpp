#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <sndfile.h>
#include <iostream>
#include <vector>
#include <QFile>
#include <QFileInfo>

class SteganographyApp : public QMainWindow {
    Q_OBJECT
public:
    SteganographyApp(QWidget *parent = nullptr) : QMainWindow(parent) {
        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        QVBoxLayout *layout = new QVBoxLayout(centralWidget);

        textInput = new QLineEdit(this);
        textInput->setPlaceholderText("Enter text to hide (or leave empty for file)");
        layout->addWidget(textInput);

        QPushButton *selectAudioButton = new QPushButton("Select Audio File", this);
        connect(selectAudioButton, &QPushButton::clicked, this, &SteganographyApp::selectAudioFile);
        layout->addWidget(selectAudioButton);

        QPushButton *selectFileButton = new QPushButton("Select File to Hide", this);
        connect(selectFileButton, &QPushButton::clicked, this, &SteganographyApp::selectInputFile);
        layout->addWidget(selectFileButton);

        QPushButton *encryptButton = new QPushButton("Hide Data", this);
        connect(encryptButton, &QPushButton::clicked, this, &SteganographyApp::encryptData);
        layout->addWidget(encryptButton);

        QPushButton *decryptButton = new QPushButton("Extract Data", this);
        connect(decryptButton, &QPushButton::clicked, this, &SteganographyApp::decryptData);
        layout->addWidget(decryptButton);

        setWindowTitle("Audio Steganography");
        resize(400, 250);
    }

private slots:
    void selectAudioFile() {
        audioFilePath = QFileDialog::getOpenFileName(this, "Select Audio File", "", "Audio Files (*.wav)");
        if (!audioFilePath.isEmpty()) {
            QMessageBox::information(this, "Success", "Audio file selected: " + audioFilePath);
        }
    }

    void selectInputFile() {
        inputFilePath = QFileDialog::getOpenFileName(this, "Select File to Hide", "", "All Files (*)");
        if (!inputFilePath.isEmpty()) {
            QMessageBox::information(this, "Success", "File selected: " + inputFilePath);
        }
    }

    void encryptData() {
        if (audioFilePath.isEmpty()) {
            QMessageBox::warning(this, "Error", "Please select an audio file first!");
            return;
        }

        QByteArray dataToHide;
        bool isText = true;
        QString fileName;

        QString text = textInput->text();
        if (!text.isEmpty()) {
            dataToHide = text.toUtf8();
            fileName = "text.txt";
        } else if (!inputFilePath.isEmpty()) {
            QFile file(inputFilePath);
            if (!file.open(QIODevice::ReadOnly)) {
                QMessageBox::warning(this, "Error", "Unable to open file for hiding!");
                return;
            }
            dataToHide = file.readAll();
            file.close();
            isText = false;
            QFileInfo fileInfo(inputFilePath);
            fileName = fileInfo.fileName();
        } else {
            QMessageBox::warning(this, "Error", "Enter text or select a file to hide!");
            return;
        }

        SF_INFO sfinfo;
        sfinfo.format = 0;
        SNDFILE *infile = sf_open(audioFilePath.toStdString().c_str(), SFM_READ, &sfinfo);
        if (!infile) {
            QMessageBox::warning(this, "Error", "Unable to open audio file!");
            return;
        }

        std::vector<short> audioData(sfinfo.frames * sfinfo.channels);
        sf_read_short(infile, audioData.data(), sfinfo.frames * sfinfo.channels);
        sf_close(infile);

        QByteArray fileNameBytes = fileName.toUtf8().left(32);
        while (fileNameBytes.size() < 32) {
            fileNameBytes.append('\0');
        }

        size_t maxBits = audioData.size();
        size_t requiredBits = (dataToHide.size() + fileNameBytes.size() + 8) * 8 + 64;
        if (requiredBits > maxBits) {
            QMessageBox::warning(this, "Error", "Audio file is too small for the data!");
            return;
        }

        std::vector<char> header;
        uint32_t dataLength = dataToHide.size();
        header.push_back((dataLength >> 24) & 0xFF);
        header.push_back((dataLength >> 16) & 0xFF);
        header.push_back((dataLength >> 8) & 0xFF);
        header.push_back(dataLength & 0xFF);
        header.push_back(isText ? 0 : 1);
        for (char c : fileNameBytes) {
            header.push_back(c);
        }

        size_t bitIndex = 0;
        for (char byte : header) {
            for (int i = 7; i >= 0 && bitIndex < audioData.size(); --i) {
                char bit = (byte >> i) & 1;
                audioData[bitIndex] = (audioData[bitIndex] & ~1) | bit;
                bitIndex++;
            }
        }

        for (char byte : dataToHide) {
            for (int i = 7; i >= 0 && bitIndex < audioData.size(); --i) {
                char bit = (byte >> i) & 1;
                audioData[bitIndex] = (audioData[bitIndex] & ~1) | bit;
                bitIndex++;
            }
        }

        QString outputPath = QFileDialog::getSaveFileName(this, "Save Audio File", "", "Audio Files (*.wav)");
        if (outputPath.isEmpty()) {
            QMessageBox::warning(this, "Error", "No save path selected!");
            return;
        }

        SNDFILE *outfile = sf_open(outputPath.toStdString().c_str(), SFM_WRITE, &sfinfo);
        if (!outfile) {
            QMessageBox::warning(this, "Error", "Unable to save audio file!");
            return;
        }
        sf_write_short(outfile, audioData.data(), audioData.size());
        sf_close(outfile);

        QMessageBox::information(this, "Success", "Data hidden in " + outputPath);
    }

    void decryptData() {
        if (audioFilePath.isEmpty()) {
            QMessageBox::warning(this, "Error", "Please select an audio file first!");
            return;
        }

        SF_INFO sfinfo;
        sfinfo.format = 0;
        SNDFILE *infile = sf_open(audioFilePath.toStdString().c_str(), SFM_READ, &sfinfo);
        if (!infile) {
            QMessageBox::warning(this, "Error", "Unable to open audio file!");
            return;
        }

        std::vector<short> audioData(sfinfo.frames * sfinfo.channels);
        sf_read_short(infile, audioData.data(), sfinfo.frames * sfinfo.channels);
        sf_close(infile);

        uint32_t dataLength = 0;
        char dataType = 0;
        QByteArray fileNameBytes;
        size_t bitIndex = 0;

        for (int i = 0; i < 4; ++i) {
            char byte = 0;
            for (int j = 0; j < 8 && bitIndex < audioData.size(); ++j) {
                byte = (byte << 1) | (audioData[bitIndex] & 1);
                bitIndex++;
            }
            dataLength = (dataLength << 8) | (unsigned char)byte;
        }

        for (int j = 0; j < 8 && bitIndex < audioData.size(); ++j) {
            dataType = (dataType << 1) | (audioData[bitIndex] & 1);
            bitIndex++;
        }

        for (int i = 0; i < 32; ++i) {
            char byte = 0;
            for (int j = 0; j < 8 && bitIndex < audioData.size(); ++j) {
                byte = (byte << 1) | (audioData[bitIndex] & 1);
                bitIndex++;
            }
            fileNameBytes.append(byte);
        }

        QString fileName = QString::fromUtf8(fileNameBytes).trimmed();

        if (dataLength * 8 + bitIndex > audioData.size()) {
            QMessageBox::warning(this, "Error", "Invalid data in audio file!");
            return;
        }

        QByteArray extractedData;
        for (size_t i = 0; i < dataLength && bitIndex < audioData.size(); ++i) {
            char byte = 0;
            for (int j = 0; j < 8 && bitIndex < audioData.size(); ++j) {
                byte = (byte << 1) | (audioData[bitIndex] & 1);
                bitIndex++;
            }
            extractedData.append(byte);
        }

        if (dataType == 0) {
            QString text = QString::fromUtf8(extractedData);
            QMessageBox::information(this, "Extracted Text", "Text: " + text);
        } else {
            QString outputPath = QFileDialog::getSaveFileName(this, "Save Extracted File", fileName, "All Files (*)");
            if (outputPath.isEmpty()) {
                QMessageBox::warning(this, "Error", "No save path selected!");
                return;
            }

            QFile file(outputPath);
            if (!file.open(QIODevice::WriteOnly)) {
                QMessageBox::warning(this, "Error", "Unable to save file!");
                return;
            }
            file.write(extractedData);
            file.close();

            QMessageBox::information(this, "Success", "File saved: " + outputPath);
        }
    }

private:
    QLineEdit *textInput;
    QString audioFilePath;
    QString inputFilePath;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    SteganographyApp window;
    window.show();
    return app.exec();
}

#include "main.moc"