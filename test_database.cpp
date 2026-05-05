#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>
#include <QFile>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    // Try to find bangs.db
    QStringList possiblePaths;
    possiblePaths << QCoreApplication::applicationDirPath() + "/bangs.db";
    possiblePaths << "./bangs.db";
    possiblePaths << "../bangs.db";
    possiblePaths << "/home/samarinara/Documents/code/lilypad/bangs.db";

    QString dbPath;
    for (const QString& path : possiblePaths) {
        if (QFile::exists(path)) {
            dbPath = path;
            qDebug() << "Found database at:" << path;
            break;
        }
    }

    if (dbPath.isEmpty()) {
        qWarning() << "Could not find bangs.db in:" << possiblePaths;
        return 1;
    }

    // Open database
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "test_connection");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qWarning() << "Failed to open database:" << db.lastError().text();
        return 1;
    }

    qDebug() << "Successfully opened database";

    // Query bangs count
    QSqlQuery query(db);
    if (!query.exec("SELECT COUNT(*) FROM bangs")) {
        qWarning() << "Failed to query:" << query.lastError().text();
        return 1;
    }

    if (query.next()) {
        qDebug() << "Total bangs in database:" << query.value(0).toInt();
    }

    // Test fl bang
    if (!query.exec("SELECT t, u FROM bangs WHERE t = 'fl'")) {
        qWarning() << "Failed to query fl bang:" << query.lastError().text();
    } else if (query.next()) {
        qDebug() << "Found fl bang - t:" << query.value(0).toString() << "u:" << query.value(1).toString();
    }

    // Test g bang
    if (!query.exec("SELECT t, u FROM bangs WHERE t = 'g'")) {
        qWarning() << "Failed to query g bang:" << query.lastError().text();
    } else if (query.next()) {
        qDebug() << "Found g bang - t:" << query.value(0).toString() << "u:" << query.value(1).toString();
    }

    db.close();
    QSqlDatabase::removeDatabase("test_connection");

    qDebug() << "Database test successful!";
    return 0;
}
