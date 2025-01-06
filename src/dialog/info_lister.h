#ifndef ZC_DIALOG_INFOLISTER_H_
#define ZC_DIALOG_INFOLISTER_H_

#include <gui/dialog.h>
#include <gui/label.h>
#include <gui/list.h>
#include <gui/window.h>
#include <gui/grid.h>
#include <zq/gui/tileanim_frame.h>
#include <initializer_list>
#include <string>
#include <set>
#include <string_view>

extern int lister_sel_val;

class BasicListerDialog: public GUI::Dialog<BasicListerDialog>
{
public:
	enum class message { REFR_INFO, OK, EDIT, EXIT, COPY, PASTE, ADV_PASTE, SAVE, LOAD, CONFIRM };
	
	BasicListerDialog(std::string title, std::string cfg_key, int start_val = 0, bool selecting = false) :
		titleTxt(title), selected_val(start_val), start_val(start_val), frozen_inds(0),
		selecting(selecting), use_preview(false), editable(true), alphabetized(false),
		cfg_key(cfg_key){};
	BasicListerDialog(std::string title, std::string cfg_key, GUI::ListData lister, int start_val = 0, bool selecting = false) :
		titleTxt(title), lister(lister), selected_val(start_val), start_val(start_val), frozen_inds(0),
		selecting(selecting), use_preview(false), editable(true), alphabetized(false),
		cfg_key(cfg_key){};
	
	std::shared_ptr<GUI::Widget> view() override;
	virtual bool handleMessage(const GUI::DialogMessage<message>& msg);
	
protected:
	virtual void preinit(){};
	virtual void postinit(){};
	virtual void update(){};
	virtual void edit(){};
	virtual void rclick(int x, int y){};
	virtual void copy(){};
	virtual bool paste(){return false;};
	virtual bool adv_paste(){return false;};
	virtual void save(){};
	virtual bool load(){return false;};
	
	virtual void add_buttons(std::shared_ptr<GUI::Grid>& cont);
	
	void resort();
	bool get_config(std::string const& name, bool default_val);
	int32_t get_config(std::string const& name, int32_t default_val);
	std::string get_config(std::string const& name, std::string const& default_val);
	double get_config(std::string const& name, double default_val);
	void set_config(std::string const& name, bool value);
	void set_config(std::string const& name, int32_t value);
	void set_config(std::string const& name, std::string const& value);
	void set_config(std::string const& name, double value);
	
	std::string titleTxt;
	GUI::ListData lister;
	int selected_val, start_val;
	size_t frozen_inds;
	bool selecting, use_preview, editable, alphabetized;
	
	std::string cfg_key;
	
	std::shared_ptr<GUI::List> widgList;
	std::shared_ptr<GUI::Label> widgInfo, copyInfo;
	std::shared_ptr<GUI::TileFrame> widgPrev;
	std::shared_ptr<GUI::Window> window;
};

class ItemListerDialog: public BasicListerDialog
{
public:
	ItemListerDialog(int itemid = -1, bool selecting = false);
	
protected:
	void preinit() override;
	void postinit() override;
	void update() override;
	void edit() override;
	void rclick(int x, int y) override;
	void copy() override;
	bool paste() override;
	bool adv_paste() override;
	void save() override;
	bool load() override;
};

class SpriteListerDialog: public BasicListerDialog
{
public:
	SpriteListerDialog(int spriteid = -1, bool selecting = false);
	
protected:
	void preinit() override;
	void postinit() override;
	void update() override;
	void edit() override;
	void rclick(int x, int y) override;
	void copy() override;
	bool paste() override;
	void save() override;
	bool load() override;
};

class SubscrWidgListerDialog: public BasicListerDialog
{
public:
	SubscrWidgListerDialog();
	
protected:
	void preinit() override;
	void postinit() override;
	void update() override;
};

class EnemyListerDialog : public BasicListerDialog
{
public:
	EnemyListerDialog(int enemyid = -1, bool selecting = false);

protected:
	void preinit() override;
	void postinit() override;
	void update() override;
	void edit() override;
	void rclick(int x, int y) override;
	void copy() override;
	bool paste() override;
	void save() override;
	bool load() override;
};

class SFXListerDialog : public BasicListerDialog
{
public:
	SFXListerDialog(int sfxid = -1, bool selecting = false);

protected:
	void preinit() override;
	void postinit() override;
	void edit() override;
};

class MidiListerDialog : public BasicListerDialog
{
public:
	MidiListerDialog(int index = -1, bool selecting = false);

protected:
	void preinit() override;
	void postinit() override;
	void update() override;
	void edit() override;
};

class StatusListerDialog : public BasicListerDialog
{
public:
	StatusListerDialog(int stat_id = -1, bool selecting = false);
	StatusListerDialog& filtered(std::function<bool(GUI::ListItem&)>&& filter)
	{
		list_filter = filter; return *this;
	}
protected:
	std::function<bool(GUI::ListItem&)> list_filter;
	
	void preinit() override;
	void postinit() override;
	void update() override;
	void edit() override;
	void editHero();
	void rclick(int x, int y) override;
	void copy() override;
	bool paste() override;
	bool pasteHero();
	bool pasteName();
	void save() override;
	bool load() override;
	void add_buttons(std::shared_ptr<GUI::Grid>& cont) override;
};

class DropDownListerDialog : public BasicListerDialog
{
public:
	DropDownListerDialog(GUI::ListData const& lister,
		std::string title, int sel = -1);
	
protected:
	void preinit() override;
	void postinit() override;
	void update() override;
};

#endif

