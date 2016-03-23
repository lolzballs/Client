#include "news.h"
#include "news_feed_widget.h"

/** Settings constructor
* Initialize the news UI
* \param p Inherited palette configuration for setting StyleSheets.
* \param parent Pointer to parent widget.
*/
News::News(QWidget* parent, QSettings* p) : QWidget(parent), p(p)
{

    this->setStyleSheet("QListWidget { background-color: " + p->value("Primary/SecondaryBase").toString() + ";} "
                        "QListWidget { color: " + p->value("Primary/LightText").toString() + "; }"
                        "QLabel { color: " + p->value("Primary/LightText").toString() + "; }"
                        "color: " + p->value("Primary/LightText").toString() + ";");
    QFont buttonFont("SourceSansPro", 9);

    rss = new QSettings(QString("rss.ini"), QSettings::IniFormat);
    manager = new QNetworkAccessManager(this);

    QVBoxLayout* newsTabLayout = new QVBoxLayout(this);

    QHBoxLayout* addRSSLayout = new QHBoxLayout();
    newsTabLayout->addLayout(addRSSLayout);

    rssAddress = new QLineEdit(this);
    addRSSLayout->addWidget(rssAddress);

    QPushButton* setRSS = new QPushButton();
    setRSS->setText("Add RSS");
    addRSSLayout->addWidget(setRSS);

    hNewsLayout = new QHBoxLayout();
    newsTabLayout->addLayout(hNewsLayout);

    loadFeeds();

    connect(setRSS, SIGNAL(clicked()), this, SLOT(setRSSFeed()));
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onRSSReturned(QNetworkReply*)));
}

News::~News()
{
}

void News::setRSSFeed()
{
    QString url = rssAddress->text();
    getRSSFeed(url);
}

void News::getRSSFeed(QString url)
{
    if(!(std::find(urls.begin(), urls.end(), url) != urls.end()))
    {
        qDebug() << "Getting RSS feed of:" << url;
        QNetworkRequest request(url.trimmed());
        manager->get(request);
        urls.push_back(url);
    }
}

void News::onRSSReturned(QNetworkReply* reply)
{
    NewsFeedWidget* newsFeedWidget = new NewsFeedWidget(this, p);
    QByteArray data = reply->readAll();
    QXmlStreamReader xml(data);
    bool atom = false;
    while (!xml.atEnd())
    {
        if (xml.isStartElement())
        {
            if (xml.name() == "channel" || xml.name() == "feed" )
            {
                if(xml.name() == "feed")atom = true;
                bool flag = false;
                while(!flag)
                {
                    xml.readNext();
                    if (xml.name() == "title")
                    {
                        QString title = xml.readElementText();
                        newsFeedWidget->setRSSTitle(title);
                        if (!rss->contains(title))
                        {
                            saveFeeds(title, reply->url().toString());
                        }
                        flag = true;
                    }
                }
            }

            if (xml.name() == "item" || xml.name() == "entry")
            {
                QString url;
                QString title;
                bool flag = false;
                while(!flag) 
                {
                    xml.readNext();
                    if (xml.name() == "title")title = xml.readElementText();
                    if (xml.name() == "link") 
                    {
                        if (atom) 
                        {
                            for (QXmlStreamAttribute attr : xml.attributes()) 
                            {
                                url = attr.value().toString();
                            }
                        }
                        else
                        {
                            url = xml.readElementText();
                        }
                    }
                    if (xml.isEndElement()) 
                    {
                        if (xml.name() == "entry" || xml.name() == "item")flag = true;
                    }
                }
                newsFeedWidget->addRSSItem(title, url);
            }
        }
        xml.readNext();
    }
    xml.clear();
    hNewsLayout->addWidget(newsFeedWidget);
    reply->close();
}

void News::saveFeeds(QString title, QString url)
{
    qDebug() << "Saving rss feed" << title << url;
    if (rss->isWritable())
    {
        rss->setValue(title, url);
    }
    qDebug() << rss->allKeys();
}

void News::loadFeeds()
{
    QStringList childKeys = rss->allKeys();
    for (int i = 0; i < childKeys.length(); i++)
    {
        QString url = rss->value(childKeys.value(i)).toString();
        getRSSFeed(url);
        urls.push_back(url);
    }
}
