//===========================================
//  Lumina Desktop Source Code
//  Copyright (c) 2016, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_main.h"
#include "ui_page_main.h"
#include "getPage.h"
#include "QDebug"
extern XDGDesktopList* APPSLIST;

//==========
//    PUBLIC
//==========
page_main::page_main(QWidget *parent) : PageWidget(parent), ui(new Ui::page_main()){
  ui->setupUi(this);
  findShort = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F), this, SLOT(showFind()));
  ui->treeWidget->setMouseTracking(true);
  ui->treeWidget->setSortingEnabled(false); //the QTreeView sort flag always puts them in backwards (reverse-alphabetical)
  connect(ui->treeWidget, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(itemTriggered(QTreeWidgetItem*, int)) );
  connect(ui->treeWidget, SIGNAL(itemPressed(QTreeWidgetItem*,int)), this, SLOT(itemTriggered(QTreeWidgetItem*, int)) );

  connect(ui->lineEdit, SIGNAL(textChanged(QString)), this, SLOT(searchChanged(QString)) );
  connect(APPSLIST, SIGNAL(appsUpdated()), this, SLOT(LoadSettings()) );
}

page_main::~page_main(){

}

void page_main::setPreviousPage(QString id){
  if(id.isEmpty()){ return; }
  for(int i=0; i<ui->treeWidget->topLevelItemCount(); i++){
    for(int j=0; j<ui->treeWidget->topLevelItem(i)->childCount(); j++){
      if(ui->treeWidget->topLevelItem(i)->child(j)->whatsThis(0)==id){
        ui->treeWidget->setCurrentItem(ui->treeWidget->topLevelItem(i)->child(j), 0);
        ui->treeWidget->scrollToItem(ui->treeWidget->topLevelItem(i)->child(j));
        return; //found item - done
      }else if(ui->treeWidget->topLevelItem(i)->child(j)->whatsThis(1)==id){
        ui->treeWidget->setCurrentItem(ui->treeWidget->topLevelItem(i)->child(j), 1);
        ui->treeWidget->scrollToItem(ui->treeWidget->topLevelItem(i)->child(j));
        return; //found item - done
      }
    }
  }

}

void page_main::UpdateItems(QString search){
  ui->treeWidget->clear();
  ui->treeWidget->setColumnCount(2);
  //Now go through and add in the known pages for each category
  QStringList SL = search.split(" "); //search list
  for(int i=0; i<INFO.length(); i++){
    if(!search.isEmpty() ){
      //See if this item needs to be included or not
      QStringList info; info << INFO[i].name.split(" ") << INFO[i].title.split(" ") << INFO[i].comment.split(" ") << INFO[i].search_tags;
      info.removeDuplicates(); //remove any duplicate terms/info first
      bool ok = true;
      for(int s=0; s<SL.length() && ok; s++){
		ok = !info.filter(SL[s]).isEmpty();
      }
      if(!ok){ continue; } //no duplicates between search terms and available info
    }
    //Test
	//if ( i != 0 && i % 2 != 0) { qDebug() << i; }
    QTreeWidgetItem *lastIt = new QTreeWidgetItem();
    lastIt->setIcon(0, LXDG::findIcon(INFO[i].icon,"") );
    lastIt->setText(0, INFO[i].name);
    lastIt->setStatusTip(0, INFO[i].comment);
    lastIt->setToolTip(0, INFO[i].comment);
    lastIt->setWhatsThis(0, INFO[i].id);
    ui->treeWidget->addTopLevelItem(lastIt);
    //qDebug() << INFO.length();
    //col++;
    //if(col>1){col = 0;}
  }
  
  ui->treeWidget->sortItems(0, Qt::AscendingOrder);
  ui->treeWidget->resizeColumnToContents(0);
  ui->treeWidget->resizeColumnToContents(1);

  //Now make sure the width of the tree widget is greater/equal to the recommended size
  int wid = ui->treeWidget->indentation() + 10;
  for(int i=0; i<ui->treeWidget->columnCount(); i++){
    wid += ui->treeWidget->columnWidth(i);
  }
  if(wid < ui->treeWidget->header()->width() ){ wid = ui->treeWidget->header()->width(); }
  //qDebug() << "Current size:" << ui->treeWidget->size() << ui->treeWidget->header()->width() << wid;
  if(ui->treeWidget->size().width() < wid ){
    // ENABLE LATER  WITH LUMINA 2.0 - Fluxbox does not like it when a window gets resized near the init routine
    //   and moves the window slightly-offscreen (titlebar hidden)
    //ui->treeWidget->setMinimumWidth( wid );
  }
}

//================
//    PUBLIC SLOTS
//================
void page_main::SaveSettings(){

}

void page_main::clearlineEdit(){
  ui->lineEdit->clear();
  ui->treeWidget->sortItems(0, Qt::AscendingOrder);
  ui->treeWidget->resizeColumnToContents(0);
  ui->treeWidget->resizeColumnToContents(1);
}


void page_main::LoadSettings(int){
  emit HasPendingChanges(false);
  emit ChangePageTitle( tr("Desktop Settings") );
  INFO.clear();
  INFO = Pages::KnownPages();
  //Also add known system setting applications to the INFO list
  QList<XDGDesktop*> apps = APPSLIST->apps(false,false); //only valid, non-hidden files
  qDebug() << "Found Apps:" << apps.length();
  for(int i=0; i<apps.length(); i++){
    if( !apps[i]->catList.contains("Settings") || apps[i]->filePath.endsWith("lumina-config.desktop") ){ continue; }
    INFO << Pages::PageInfo(apps[i]->filePath, apps[i]->name, apps[i]->genericName, apps[i]->icon, apps[i]->comment, "system", QStringList(), apps[i]->keyList);
  }
  //Now sort the items according to the translated name
  QStringList names;
  for(int i=0; i<INFO.length(); i++){ names << INFO[i].name; }
  names.sort(Qt::CaseInsensitive);
  QList<PAGEINFO> sorted;
  for(int i=0; i<names.length(); i++){
    for(int j=0; j<INFO.length(); j++){
      if(INFO[j].name==names[i]){ sorted << INFO.takeAt(j); break; }
    }
  }
  INFO = sorted; //replace the internal list with the sorted version
  UpdateItems(ui->lineEdit->text());
  ui->lineEdit->setFocus();
}

void page_main::updateIcons(){
  UpdateItems("");
}

//=================
//    PRIVATE SLOTS
//=================
void page_main::showFind(){
  ui->lineEdit->setFocus();
  ui->treeWidget->setCurrentItem(0);
}

void page_main::itemTriggered(QTreeWidgetItem *it, int col){
  if(it->childCount()>0){
    it->setExpanded( !it->isExpanded() );
    it->setSelected(false);
  }else if(!it->whatsThis(col).isEmpty()){
    QString id = it->whatsThis(col);
    if(id.endsWith(".desktop")){ QProcess::startDetached("lumina-open \""+id+"\""); } //external setting utility
    else{ emit ChangePage(it->whatsThis(col)); } //internal page
  }else{
   it->setSelected(false);
  }
}

void page_main::searchChanged(QString txt){
  UpdateItems(txt.simplified());
}
