#include "info_comics_view.h"

#include <QtQuick>

#include "comic.h"
#include "comic_files_manager.h"
#include "comic_model.h"
#include "comic_db.h"
#include "yacreader_comic_info_helper.h"
#include "yacreader_comics_selection_helper.h"
#include "theme.h"

#include "QsLog.h"

InfoComicsView::InfoComicsView(QWidget *parent)
    :ComicsView(parent)
{
    qmlRegisterType<ComicModel>("com.yacreader.ComicModel",1,0,"ComicModel");
    qmlRegisterType<ComicDB>("com.yacreader.ComicDB",1,0,"ComicDB");
    qmlRegisterType<ComicInfo>("com.yacreader.ComicInfo",1,0,"ComicInfo");

    view = new QQuickView();
    connect(
        view, &QQuickView::statusChanged,
        [=] (QQuickView::Status status)
        {
            if (status == QQuickView::Error)
            {
                QLOG_ERROR() << view->errors();
            }
        }
      );

    container = QWidget::createWindowContainer(view, this);

    container->setFocusPolicy(Qt::StrongFocus);

    QQmlContext *ctxt = view->rootContext();

    auto theme = Theme::currentTheme();

    theme.configureInfoView(ctxt);

    view->setSource(QUrl("qrc:/qml/InfoComicsView.qml"));

    QObject *rootObject = dynamic_cast<QObject*>(view->rootObject());
    flow = rootObject->findChild<QObject*>("flow");
    list = rootObject->findChild<QObject*>("list");

    connect(flow, SIGNAL(currentCoverChanged(int)), this, SLOT(updateInfoForIndex(int)));
    connect(flow, SIGNAL(currentCoverChanged(int)), this, SLOT(setCurrentIndex(int)));

    selectionHelper = new YACReaderComicsSelectionHelper(this);
    comicInfoHelper = new YACReaderComicInfoHelper(this);

    QVBoxLayout * l = new QVBoxLayout;
    l->addWidget(container);
    this->setLayout(l);

    setContentsMargins(0,0,0,0);
    l->setContentsMargins(0,0,0,0);
    l->setSpacing(0);

    setShowMarks(true);

    QLOG_TRACE() << "GridComicsView";
}

InfoComicsView::~InfoComicsView()
{
    delete view;
}

void InfoComicsView::setToolBar(QToolBar *toolBar)
{
    static_cast<QVBoxLayout *>(this->layout())->insertWidget(1,toolBar);
    this->toolbar = toolBar;
}

void InfoComicsView::setModel(ComicModel *model)
{
    if(model == NULL)
        return;

    selectionHelper->setModel(model);
    comicInfoHelper->setModel(model);

    ComicsView::setModel(model);

    QQmlContext *ctxt = view->rootContext();

    /*if(_selectionModel != NULL)
        delete _selectionModel;

    _selectionModel = new QItemSelectionModel(model);*/

    ctxt->setContextProperty("comicsList", model);
    if(model->rowCount()>0)
        ctxt->setContextProperty("backgroundImage", this->model->data(this->model->index(0, 0), ComicModel::CoverPathRole));
    else
        ctxt->setContextProperty("backgroundImage", QUrl());

    ctxt->setContextProperty("comicsSelection", selectionHelper->selectionModel());
    ctxt->setContextProperty("contextMenuHelper",this);
    ctxt->setContextProperty("currentIndexHelper", this);
    ctxt->setContextProperty("comicInfoHelper", comicInfoHelper);
    /*ctxt->setContextProperty("comicsSelectionHelper", this);
    ctxt->setContextProperty("dragManager", this);*/
    ctxt->setContextProperty("dropManager", this);


    if(model->rowCount()>0)
    {
        setCurrentIndex(model->index(0,0));
        updateInfoForIndex(0);
    }
}

void InfoComicsView::setCurrentIndex(const QModelIndex &index)
{
    QQmlProperty(list, "currentIndex").write(index.row());

    selectionHelper->clear();
    selectionHelper->selectIndex(index.row());
}

void InfoComicsView::setCurrentIndex(int index)
{
    selectionHelper->clear();
    selectionHelper->selectIndex(index);
}

QModelIndex InfoComicsView::currentIndex()
{
    return selectionHelper->currentIndex();
}

QItemSelectionModel *InfoComicsView::selectionModel()
{
    return selectionHelper->selectionModel();
}

void InfoComicsView::scrollTo(const QModelIndex &mi, QAbstractItemView::ScrollHint hint)
{
    Q_UNUSED(mi);
    Q_UNUSED(hint);
}

void InfoComicsView::toFullScreen()
{
    toolbar->hide();
}

void InfoComicsView::toNormal()
{
    toolbar->show();
}

void InfoComicsView::updateConfig(QSettings *settings)
{
    Q_UNUSED(settings);
}

void InfoComicsView::enableFilterMode(bool enabled)
{
    Q_UNUSED(enabled);
}

void InfoComicsView::selectIndex(int index)
{
    selectionHelper->selectIndex(index);
}

void InfoComicsView::updateCurrentComicView()
{

}

void InfoComicsView::setShowMarks(bool show)
{
    QQmlContext *ctxt = view->rootContext();
    ctxt->setContextProperty("show_marks", show);
}

void InfoComicsView::selectAll()
{
    selectionHelper->selectAll();
}

bool InfoComicsView::canDropUrls(const QList<QUrl> &urls, Qt::DropAction action)
{
    if(action == Qt::CopyAction)
    {
        QString currentPath;
        foreach (QUrl url, urls)
        {
            //comics or folders are accepted, folders' content is validate in dropEvent (avoid any lag before droping)
            currentPath = url.toLocalFile();
            if(Comic::fileIsComic(currentPath) || QFileInfo(currentPath).isDir())
                return true;
        }
    }
    return false;
}

void InfoComicsView::droppedFiles(const QList<QUrl> &urls, Qt::DropAction action)
{
    bool validAction = action == Qt::CopyAction; //TODO add move

    if(validAction)
    {
        QList<QPair<QString, QString> > droppedFiles = ComicFilesManager::getDroppedFiles(urls);
        emit copyComicsToCurrentFolder(droppedFiles);
    }
}

void InfoComicsView::requestedContextMenu(const QPoint &point)
{
    emit customContextMenuViewRequested(point);
}

void InfoComicsView::selectedItem(int index)
{
    emit selected(index);
}
