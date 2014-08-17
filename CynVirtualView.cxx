#include <string>
#include <FL/fl_draw.H>
#include <CynVirtualView.h>
#include <N_Threads.h>
#include <N_TextUtil.h>

using namespace ab;

namespace afltk {
	void Scrollbar_CB(Fl_Widget* w, void *p)
	{
		CynVirtualView* view = (CynVirtualView*)p;
		V_PageScrollbar* scroll = (V_PageScrollbar*)w;
		view->filePos = (long long)((double)scroll->value() / (scroll->maximum() - scroll->minimum())*view->mapObj->filesize());
		view->lineFilePos = (int)(view->numVisibleLines*(double)scroll->value() / (scroll->maximum() - scroll->minimum()));
		view->redraw();
	}

	void Scrollbar_CB1(Fl_Widget* w, void *p, VPS_Increment* inc)
	{
		//CynVirtualView* view = (CynVirtualView*)p;
		//printf("cb\n");
		//Scrollbar_CB(w, p);
	}

	/// Constructor.
	CynVirtualView::CynVirtualView(int X, int Y, int W, int H, const char *L) : Fl_Group(X, Y, W, H, L) {
		_vscroll = new V_PageScrollbar(X+W-16, Y, 16, H-16);		// will be resized by draw()
		_vscroll->type(FL_VERTICAL);
		_vscroll->maximum(100);
		_vscroll->linesize(1);
		_vscroll->pagesize(20);
		_vscroll->callback(Scrollbar_CB, (void*)this);
		_vscroll->callbackScroll(Scrollbar_CB1, (void*)this);
		_hscroll = new Fl_Scrollbar(X, Y+H-16, W-16, 16);		// will be resized by draw()
		_hscroll->type(FL_HORIZONTAL);
		_hscroll->maximum(100);

		Bom_type = NO_BOM;
		coding = 0;
		lines = new T_List<char*>;
		numVisibleLines = getNumVisibleLines();
		mapObj = NULL;
	}

	/// Destructor.
	CynVirtualView::~CynVirtualView() {
		delete mapObj;
		delete lines;
		delete _vscroll;
		delete _hscroll;
	}

	int CynVirtualView::handle(int event)
	{
		int ret = Fl_Group::handle(event);
		return 1;
	}

	int CynVirtualView::findFirstVisibleLine()
	{
		int i = startmappos;
		int line = lineFilePos;
		backToBeginLine(map, i);
		while (line>0)
		{
			i--;
			backToBeginLine(map, i);
			line--;
		}
		if (mappos==0 && Bom_type==BOM_UTF8 && i<sizeof(BOM_UTF8_DATA))
			i=sizeof(BOM_UTF8_DATA);
		return i;
	}

	void CynVirtualView::draw()
	{
		numVisibleLines = getNumVisibleLines();
		update_map();
		int pos = findFirstVisibleLine();
		int pos0 = pos;
		int ymax = y()+h() - 16; //-16 for _hscroll height afer _hscroll resize
		char *line;
		int blockLine = 0;
		int endType;
		int posY = y();
		while (posY < ymax && getNextLine(map, current_mapsize, line, pos, endType, MaxLineLen, coding==CODING_UTF8))
		{
			lines->add(line);
			posY += 16;
		}
		fl_font(FL_COURIER, 12);
		for (int i = 0; i < lines->size(); i++)
		{
			fl_rectf(x(), y()+i*16, w()-16, 16, 255, 255, 255);
			fl_color(0, 0, 0);//font color
			fl_draw(lines->at(i), min(100,strlen(lines->at(i))), x() + 5, y() + i * 16 + 12);
		}
		for (int i = 0; i < lines->size(); i++)
			free(lines->at(i));
		lines->clear();
		fl_rectf(x(),posY, w(), y()+h()-16-posY, 255, 255, 255);//draw remaining area
		_vscroll->slider_size( (double)(pos-pos0) / mapObj->filesize() );
     	_vscroll->resize(x() + w() - 16, y(), 16, h() - 16);
		draw_child(*_vscroll);
		_hscroll->resize(x(), y()+h()-16, w()-16, 16);
		draw_child(*_hscroll);
	}

	void CynVirtualView::setFile(const wchar_t *fileName)
	{
		init_map(fileName);
		filePos = 0;
		lineFilePos = 0;
		_vscroll->value(0);
	}

    void CynVirtualView::init_map(const wchar_t *fileName)
	{
		delete mapObj;
		request_mapsize = 256*1024;
		mapObj = new N_Mapping(fileName, request_mapsize);
		map = (char*)mapObj->map(0);
		current_mapsize = mapObj->mapsize();
		determineCoding();
	}

	void CynVirtualView::update_map()
	{
		if (request_mapsize>=mapObj->filesize() || filePos<=request_mapsize/2)
		{
			mappos = 0;
		}
		else
		{
			assert(request_mapsize>=4*mapObj->granul());
			assert(filePos>request_mapsize/2);
			mappos = (filePos-request_mapsize/2)/mapObj->granul()*mapObj->granul();
		}
		assert(filePos>=mappos && filePos-mappos<request_mapsize);
		startmappos = (int)(filePos-mappos);
		map = (char*)mapObj->map(mappos);
		current_mapsize = mapObj->mapsize();
	}

	void CynVirtualView::determineCoding()
	{
		if (current_mapsize >= sizeof(BOM_UTF8_DATA))
		{
			if (memcmp(map, BOM_UTF8_DATA, sizeof(BOM_UTF8_DATA)) == 0)
				Bom_type = BOM_UTF8;
			else
				Bom_type = NO_BOM;
		}
		else Bom_type = NO_BOM;
		if (Bom_type == BOM_UTF8)
			coding = CODING_UTF8;
		else if (isUTF8(map, current_mapsize))
			coding = CODING_UTF8;
		else
			coding = CODING_LOCALE;
	}
}
