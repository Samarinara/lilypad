#include "urlprocessor.h"
#include <QUrl>
#include <QRegularExpression>
#include <QFile>
#include <QCoreApplication>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QStringList>

UrlProcessor::UrlProcessor(QObject* parent)
    : QObject(parent)
{
    static int connectionCounter = 0;
    QString connectionName = QString("bangs_connection_%1").arg(++connectionCounter);
    m_db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    initDatabase();
    loadBangData();
}

UrlProcessor::~UrlProcessor()
{
    QString connectionName = m_db.connectionName();
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
}

void UrlProcessor::initDatabase()
{
    QString connectionName = m_db.connectionName();

    QStringList possiblePaths;
    possiblePaths << QCoreApplication::applicationDirPath() + "/bangs.db";
    possiblePaths << "./bangs.db";
    possiblePaths << "../bangs.db";
    possiblePaths << "../../bangs.db";

    QString dbPath;
    for (const QString& path : possiblePaths) {
        if (QFile::exists(path)) {
            dbPath = path;
            break;
        }
    }

    if (dbPath.isEmpty()) {
        qWarning() << "Could not find bangs.db in:" << possiblePaths;
        return;
    }

    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "Failed to open bangs database:" << m_db.lastError().text();
    } else {
        qDebug() << "Opened bangs database:" << dbPath;
    }
}

void UrlProcessor::loadBangData()
{
    m_bangTable.clear();
    m_suffixTable.clear();

    if (!m_db.isOpen()) {
        return;
    }

    QSqlQuery query(m_db);
    if (!query.exec("SELECT t, s, u FROM bangs")) {
        qWarning() << "Failed to query bangs:" << query.lastError().text();
        return;
    }

    while (query.next()) {
        QString t = query.value(0).toString();
        QString s = query.value(1).toString();
        QString u = query.value(2).toString();

        if (!t.isEmpty() && !u.isEmpty()) {
            m_bangTable[t] = u;
        }

        if (!s.isEmpty() && !u.isEmpty()) {
            m_suffixTable[s.toLower()] = u;
        }
    }
}

QUrl UrlProcessor::process(const QString& input)
{
    QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) {
        return QUrl();
    }

    if (isUrl(trimmed)) {
        QString urlStr = trimmed;
        if (!urlStr.contains("://")) {
            urlStr = "https://" + urlStr;
        }
        return QUrl(urlStr);
    }

    QUrl bangUrl = handleBangRouting(trimmed);
    if (!bangUrl.isEmpty()) {
        return bangUrl;
    }

    QUrl suffixUrl = handleSuffixRouting(trimmed);
    if (!suffixUrl.isEmpty()) {
        return suffixUrl;
    }

    return fallbackRouting(trimmed);
}

bool UrlProcessor::isUrl(const QString& input)
{
    if (input.contains("://")) {
        return true;
    }

    if (input.contains('.') && !input.contains(' ')) {
        static const QRegularExpression domainRegex(R"(^[a-zA-Z0-9][-a-zA-Z0-9]*(\.[a-zA-Z0-9][-a-zA-Z0-9]*)+$)");
        if (domainRegex.match(input).hasMatch()) {
            return true;
        }
    }

    return false;
}

QUrl UrlProcessor::handleBangRouting(const QString& query)
{
    static const QRegularExpression bangRegex(R"(!(\w+))");
    QRegularExpressionMatch match = bangRegex.match(query);

    if (match.hasMatch()) {
        QString bang = match.captured(1);
        QString bangFull = match.captured(0);

        if (m_bangTable.contains(bang)) {
            QString urlTemplate = m_bangTable[bang];
            QString searchQuery = query;
            searchQuery.remove(bangFull);
            searchQuery = searchQuery.trimmed();

            // If no search query, go to site root
            if (searchQuery.isEmpty()) {
                QUrl templateUrl(urlTemplate);
                QUrl rootUrl;
                rootUrl.setScheme(templateUrl.scheme());
                rootUrl.setHost(templateUrl.host());
                return rootUrl;
            }

            QString finalUrl = urlTemplate;
            finalUrl.replace("{{{s}}}", urlEncode(searchQuery));
            return QUrl(finalUrl);
        }
    }

    return QUrl();
}

QUrl UrlProcessor::handleSuffixRouting(const QString& query)
{
    if (query.contains('!')) {
        return QUrl();
    }

    QStringList parts = query.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 2) {
        // Need at least 2 parts: query + suffix
        return QUrl();
    }

    QString lastWord = parts.last().toLower();

    if (m_suffixTable.contains(lastWord)) {
        QString urlTemplate = m_suffixTable[lastWord];

        parts.removeLast();
        QString searchQuery = parts.join(" ");

        // If no search query, go to site root
        if (searchQuery.isEmpty()) {
            QUrl templateUrl(urlTemplate);
            QUrl rootUrl;
            rootUrl.setScheme(templateUrl.scheme());
            rootUrl.setHost(templateUrl.host());
            return rootUrl;
        }

        QString finalUrl = urlTemplate;
        finalUrl.replace("{{{s}}}", urlEncode(searchQuery));
        return QUrl(finalUrl);
    }

    return QUrl();
}

QUrl UrlProcessor::fallbackRouting(const QString& query)
{
    if (m_bangTable.contains("fl")) {
        QString urlTemplate = m_bangTable["fl"];
        QString finalUrl = urlTemplate;
        finalUrl.replace("{{{s}}}", urlEncode(query));
        return QUrl(finalUrl);
    }

    return QUrl(query);
}

QString UrlProcessor::urlEncode(const QString& str)
{
    return QUrl::toPercentEncoding(str, "", " ");
}
