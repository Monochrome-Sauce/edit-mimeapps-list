#include "widget.h"
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QTreeWidget>
#include <QListWidget>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QStandardPaths>
#include <QPushButton>
#include <QMessageBox>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    QDir applicationsDir("/usr/share/applications/");

    for (const QFileInfo &file : applicationsDir.entryInfoList(QStringList("*.desktop"), QDir::Files)) {
        loadDesktopFile(file);
    }

    QHBoxLayout *mainLayout = new QHBoxLayout;
    setLayout(mainLayout);
    m_applicationList = new QTreeWidget;
    mainLayout->addWidget(m_applicationList);

    QGridLayout *rightLayout = new QGridLayout;
    mainLayout->addLayout(rightLayout);

    m_setDefaultButton = new QPushButton(tr("Set as default application for these file types"));
    m_setDefaultButton->setEnabled(false);

    m_mimetypeList = new QListWidget;
    m_mimetypeList->setSelectionMode(QAbstractItemView::MultiSelection);

    rightLayout->addWidget(m_mimetypeList);
    rightLayout->addWidget(m_setDefaultButton);

    QStringList types = m_applications.uniqueKeys();
    std::sort(types.begin(), types.end());
    for (const QString &type : types)  {
        QTreeWidgetItem *typeItem = new QTreeWidgetItem(QStringList(type));
        QStringList applications = m_applications[type].toList();
        std::sort(applications.begin(), applications.end());
        for (const QString &application : applications) {
            QTreeWidgetItem *appItem = new QTreeWidgetItem(QStringList(m_applicationNames[application]));
            appItem->setData(0, Qt::UserRole, application);
            appItem->setIcon(0, QIcon::fromTheme(m_applicationIcons[application]));
            typeItem->addChild(appItem);
        }
        m_applicationList->addTopLevelItem(typeItem);
    }
    m_applicationList->setHeaderHidden(true);

    connect(m_applicationList, &QTreeWidget::itemSelectionChanged, this, &Widget::onMimetypeSelected);
    connect(m_setDefaultButton, &QPushButton::clicked, this, &Widget::onSetDefaultClicked);
}

Widget::~Widget()
{

}

void Widget::onMimetypeSelected()
{
    m_setDefaultButton->setEnabled(false);
    m_mimetypeList->clear();

    QList<QTreeWidgetItem*> selectedItems = m_applicationList->selectedItems();
    if (selectedItems.count() != 1) {
        return;
    }

    const QTreeWidgetItem *item = selectedItems.first();
    if (!item->parent()) {
        return;
    }

    const QString mimetypeGroup = item->parent()->text(0);
    const QString application = item->data(0, Qt::UserRole).toString();

    for (const QString &supportedMime : m_supportedMimetypes.values(application)) {
        if (!supportedMime.startsWith(mimetypeGroup)) {
            continue;
        }
        const QMimeType mimetype = m_mimeDb.mimeTypeForName(supportedMime);
        QString name = mimetype.filterString().trimmed();
        if (name.isEmpty()) {
            name = mimetype.comment().trimmed();
        }
        if (name.isEmpty()) {
            name = mimetype.name().trimmed();
        }
        QListWidgetItem *item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, mimetype.name());
        item->setIcon(QIcon::fromTheme(mimetype.iconName()));
        m_mimetypeList->addItem(item);
        item->setSelected(true);
    }

    m_setDefaultButton->setEnabled(m_mimetypeList->count() > 0);
}

void Widget::onSetDefaultClicked()
{
    QList<QTreeWidgetItem*> selectedItems = m_applicationList->selectedItems();
    if (selectedItems.count() != 1) {
        return;
    }

    const QTreeWidgetItem *item = selectedItems.first();
    if (!item->parent()) {
        return;
    }

    const QString application = item->data(0, Qt::UserRole).toString();
    if (application.isEmpty()) {
        return;
    }

    QSet<QString> unselected;
    QSet<QString> selected;
    for (int i=0; i<m_mimetypeList->count(); i++) {
        QListWidgetItem *item = m_mimetypeList->item(i);
        const QString name = item->data(Qt::UserRole).toString();
        if (item->isSelected()) {
            selected.insert(name);
        } else {
            unselected.insert(name);
        }
    }

    setDefault(application, selected, unselected);
}

void Widget::loadDesktopFile(const QFileInfo &fileInfo)
{
    // Ugliest implementation of .desktop file reading ever

    QFile file(fileInfo.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open" << fileInfo.fileName();
        return;
    }

    QStringList mimetypes;
    QString appName;

    bool inCorrectGroup = false;
    while (!file.atEnd()) {
        QString line = file.readLine().simplified();

        if (line.startsWith('[')) {
            inCorrectGroup = (line == "[Desktop Entry]");
            continue;
        }

        if (!inCorrectGroup) {
            continue;
        }

        if (line.startsWith("MimeType")) {
            line.remove(0, line.indexOf('=') + 1);
            mimetypes = line.split(';', QString::SkipEmptyParts);
            continue;
        }

        if (line.startsWith("Name") && !line.contains('[')) {
            line.remove(0, line.indexOf('=') + 1);
            appName = line;
            continue;
        }

        if (line.startsWith("Icon")) {
            line.remove(0, line.indexOf('=') + 1);
            m_applicationIcons[fileInfo.fileName()] = line;
            continue;
        }

        if (line.startsWith("NoDisplay=") && line.contains("true", Qt::CaseInsensitive)) {
            return;
        }
    }

    if (!appName.isEmpty()) {
        m_applicationNames.insert(fileInfo.fileName(), appName);
    } else {
        qWarning() << "Missing name" << fileInfo.fileName();
    }

    if (mimetypes.isEmpty()) {
        return;
    }

    for (const QString &readMimeName : mimetypes) {
        // Resolve aliases etc
        const QMimeType mimetype = m_mimeDb.mimeTypeForName(readMimeName.trimmed());
        if (!mimetype.isValid()) {
            continue;
        }

        const QString name = mimetype.name();
        if (m_supportedMimetypes.contains(fileInfo.fileName(), name)) {
            continue;
        }

        const QStringList parts = name.split('/');
        if (parts.count() != 2) {
            continue;
        }

        const QString type = parts[0].trimmed();

        m_applications[type].insert(fileInfo.fileName());
        m_supportedMimetypes.insert(fileInfo.fileName(), name);
    }
}

void Widget::setDefault(const QString &appName, const QSet<QString> &mimetypes, const QSet<QString> &unselectedMimetypes)
{
    const QString filePath = QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)).absoluteFilePath("mimeapps.list");
    QFile file(filePath);

    // Read in existing mimeapps.list, skipping the lines for the mimetypes we're updating
    QList<QByteArray> existingContent;
    QList<QByteArray> existingAssociations;
    if (file.open(QIODevice::ReadOnly)) {
        bool inCorrectGroup = false;
        while (!file.atEnd()) {
            const QByteArray line = file.readLine().trimmed();

            if (line.isEmpty()) {
                continue;
            }

            if (line.startsWith('[')) {
                inCorrectGroup = (line == "[Default Applications]");
                if (!inCorrectGroup) {
                    existingContent.append(line);
                }
                continue;
            }

            if (!inCorrectGroup) {
                existingContent.append(line);
                continue;
            }

            if (!line.contains('=')) {
                existingAssociations.append(line);
                continue;
            }

            const QString mimetype = m_mimeDb.mimeTypeForName(line.split('=').first().trimmed()).name();
            if (!mimetypes.contains(mimetype) && !unselectedMimetypes.contains(mimetype)) {
                existingAssociations.append(line);
            }
        }

        file.close();
    } else {
        qDebug() << "Unable to open file for reading";
    }

    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Failed to store settings"), file.errorString());
        return;
    }

    for (const QByteArray &line : existingContent) {
        file.write(line + '\n');
    }
    file.write("\n[Default Applications]\n");
    for (const QByteArray &line : existingAssociations) {
        file.write(line + '\n');
    }

    for (const QString &mimetype : mimetypes) {
        file.write(QString(mimetype + '=' + appName + '\n').toUtf8());
    }

    return;
}
