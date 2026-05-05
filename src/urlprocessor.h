// ============================================================================
// URLPROCESSOR.H
// ============================================================================
#ifndef LILYPAD_URLPROCESSOR_H
#define LILYPAD_URLPROCESSOR_H

#include <QObject>
#include <QUrl>
#include <QString>
#include <QHash>
#include <QRegularExpression>
#include <QSqlDatabase>

class UrlProcessor : public QObject {
    Q_OBJECT

public:
    explicit UrlProcessor(QObject* parent = nullptr);
    ~UrlProcessor();

    Q_INVOKABLE QUrl process(const QString& input);

private:
    QHash<QString, QString> m_bangTable;
    QHash<QString, QString> m_suffixTable;
    QSqlDatabase m_db;

    void initDatabase();
    void loadBangData();
    bool isUrl(const QString& input);
    QUrl handleBangRouting(const QString& query);
    QUrl handleSuffixRouting(const QString& query);
    QUrl fallbackRouting(const QString& query);
    static QString urlEncode(const QString& str);
};

#endif // LILYPAD_URLPROCESSOR_H
