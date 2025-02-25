#pragma once

#include <QtWidgets/QListView>
#include <QtWidgets/QStyledItemDelegate>
#include <mutex>
#include "viewframe.h"
#include "filter.h"
#include "uicontext.h"

#define STRINGS_LIST_UPDATE_INTERVAL 250

class BINARYNINJAUIAPI StringsListModel : public QAbstractItemModel, public BinaryNinja::BinaryDataNotification
{
	Q_OBJECT

	struct StringUpdateEvent
	{
		BNStringReference ref;
		bool added;
	};

	QWidget* m_stringsList;
	BinaryViewRef m_data;
	std::vector<BNStringReference> m_allStrings;
	std::vector<BNStringReference> m_strings;
	std::string m_filter;

	std::mutex m_updateMutex;
	std::vector<StringUpdateEvent> m_updates;

	bool m_excludeStringsInBasicBlocks;
	bool m_excludeUnreferencedStrings;

	static bool stringComparison(const BNStringReference& a, const BNStringReference& b);
	bool matchString(const BNStringReference& stringRef);

	std::vector<StringUpdateEvent> getQueuedStringUpdates();

  public:
	StringsListModel(QWidget* parent, BinaryViewRef data);
	virtual ~StringsListModel();

	virtual QModelIndex index(int row, int col, const QModelIndex& parent) const override;
	virtual QModelIndex parent(const QModelIndex& i) const override;
	virtual bool hasChildren(const QModelIndex& parent) const override;
	virtual int rowCount(const QModelIndex& parent) const override;
	virtual int columnCount(const QModelIndex& parent) const override;
	virtual QVariant data(const QModelIndex& i, int role) const override;

	BNStringReference getStringAt(const QModelIndex& i);
	QModelIndex findString(const BNStringReference& ref);

	virtual void OnStringFound(BinaryNinja::BinaryView* data, BNStringType type, uint64_t offset, size_t len) override;
	virtual void OnStringRemoved(
	    BinaryNinja::BinaryView* data, BNStringType type, uint64_t offset, size_t len) override;
	void updateStrings();

	void setFilter(const std::string& filter);

	void updateFilter() { setFilter(m_filter); };

	void toggleExcludeStringsInBasicBlocks() { m_excludeStringsInBasicBlocks = !m_excludeStringsInBasicBlocks; };
	void toggleExcludeUnreferencedStrings() { m_excludeUnreferencedStrings = !m_excludeUnreferencedStrings; };

	void excludeStringsInBasicBlocks(bool exclude) { m_excludeStringsInBasicBlocks = exclude; };
	void excludeUnreferencedStrings(bool exclude) { m_excludeUnreferencedStrings = exclude; };

	bool getExcludeStringsInBasicBlocks() const { return m_excludeStringsInBasicBlocks; };
	bool getExcludeUnreferencedStrings() const { return m_excludeUnreferencedStrings; };
};

class BINARYNINJAUIAPI StringItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT

	QWidget* m_owner;
	QFont m_font;
	int m_baseline, m_charWidth, m_charHeight, m_charOffset;

	void initFont();

  public:
	StringItemDelegate(QWidget* parent);

	void updateFonts();

	virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& idx) const override;
	virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& idx) const override;
	QFont getFont() const { return m_font; }
};

class StringsContainer;
class StringsViewSidebarWidget;

class BINARYNINJAUIAPI StringsView : public QListView, public View, public FilterTarget
{
	Q_OBJECT

	BinaryViewRef m_data;
	ViewFrame* m_view;
	StringsContainer* m_container;

	StringsListModel* m_list;
	StringItemDelegate* m_itemDelegate;
	QTimer* m_updateTimer;

	uint64_t m_selectionBegin, m_selectionEnd;

  public:
	StringsView(BinaryViewRef data, ViewFrame* view, StringsContainer* container);

	virtual BinaryViewRef getData() override { return m_data; }
	virtual uint64_t getCurrentOffset() override;
	virtual BNAddressRange getSelectionOffsets() override;
	virtual void setSelectionOffsets(BNAddressRange range) override;
	virtual bool navigate(uint64_t offset) override;

	virtual void updateFonts() override;

	virtual StatusBarWidget* getStatusBarWidget() override;

	virtual void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;

	virtual void setFilter(const std::string& filter) override;
	virtual void scrollToFirstItem() override;
	virtual void scrollToCurrentItem() override;
	virtual void selectFirstItem() override;
	virtual void activateFirstItem() override;
	virtual QFont getFont() override { return m_itemDelegate->getFont(); }

	bool getExcludeStringsInBasicBlocks() const { return m_list->getExcludeStringsInBasicBlocks(); };
	bool getExcludeUnreferencedStrings() const { return m_list->getExcludeUnreferencedStrings(); };

	void toggleExcludeStringsInBasicBlocks() const { m_list->toggleExcludeStringsInBasicBlocks(); };
	void toggleExcludeUnreferencedStrings() const { m_list->toggleExcludeUnreferencedStrings(); };

	void copyText();
	virtual bool canCopy() override;

  protected:
	virtual void keyPressEvent(QKeyEvent* event) override;
	virtual bool event(QEvent* event) override;

  private Q_SLOTS:
	void goToString(const QModelIndex& idx);
	void updateTimerEvent();
};

class BINARYNINJAUIAPI StringsContainer : public QWidget, public ViewContainer
{
	Q_OBJECT

	friend class StringsView;

	ViewFrame* m_view;
	StringsView* m_strings;
	FilteredView* m_filter;
	FilterEdit* m_separateEdit = nullptr;
	StringsViewSidebarWidget* m_widget;

  public:
	StringsContainer(BinaryViewRef data, ViewFrame* view, StringsViewSidebarWidget* parent, bool separateEdit = false);
	virtual View* getView() override { return m_strings; }

	StringsView* getStringsView() { return m_strings; }
	FilteredView* getFilter() { return m_filter; }
	FilterEdit* getSeparateFilterEdit() { return m_separateEdit; }

  protected:
	virtual void focusInEvent(QFocusEvent* event) override;
};

class StringsViewType : public ViewType
{
	static StringsViewType* m_instance;

  public:
	StringsViewType();
	virtual int getPriority(BinaryViewRef data, const QString& filename);
	virtual QWidget* create(BinaryViewRef data, ViewFrame* viewFrame);
	static void init();
};


class BINARYNINJAUIAPI StringsViewSidebarWidget : public SidebarWidget
{
	Q_OBJECT

	friend class StringsView;

	QWidget* m_header;
	StringsContainer* m_container;

  public:
	StringsViewSidebarWidget(BinaryViewRef data, ViewFrame* frame);
	virtual QWidget* headerWidget() override { return m_header; }
	virtual void focus() override;

  protected:
	virtual void contextMenuEvent(QContextMenuEvent* event) override;

  private Q_SLOTS:
	void showContextMenu();
};


class BINARYNINJAUIAPI StringsViewSidebarWidgetType : public SidebarWidgetType
{
  public:
	StringsViewSidebarWidgetType();
	virtual SidebarWidget* createWidget(ViewFrame* frame, BinaryViewRef data) override;
};
