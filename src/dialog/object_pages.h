#ifndef ZC_DIALOG_OBJECTPAGES_H_
#define ZC_DIALOG_OBJECTPAGES_H_

#include "base/zc_alleg.h"
#include "base/ints.h"
#include "dialog/externs.h"
#include <functional>

using std::pair;

#define OBJPG_CB_RECTSEL    0
#define OBJPG_CB_COLMODE    1
#define OBJPG_CB_NUM_GLOBAL 2
#define OBJPG_CB_INDIVIDUAL 1
#define OBJPG_CB_TOTAL (OBJPG_CB_NUM_GLOBAL+OBJPG_CB_INDIVIDUAL)
class ObjectTemplate
{
	static constexpr uint lmargin = 6, rmargin = 6, tmargin = 25, bmargin = 6;
	static constexpr uint window_w = 640+lmargin+rmargin, window_h = 500+lmargin+rmargin;
	
	static constexpr uint gridhei = 13*32; // 416
	static constexpr uint panelx = lmargin, panely = tmargin+gridhei+bmargin;
	static constexpr uint panelrx = window_w-rmargin, panelby = window_h-bmargin;
	static constexpr uint panelcx = (panelx+panelrx)/2, panelcy = (panely+panelby)/2;
	
	static constexpr uint cpyx = panelx+68, cpyy = panelcy-(32/2);
	static constexpr uint cpytxt_x = cpyx-2-4;
	static constexpr uint curx = cpyx+32+4, cury = panelcy-(32/2);
	static constexpr uint curtxt_x = curx+32+2+8;
	static constexpr uint csettxt_x = curtxt_x+70;
	
	static constexpr uint cbox_x = csettxt_x+50, cbox_y = panely+2, cbox_h = 16;
	
	static constexpr uint pgx = panelrx-14;
	
	static constexpr uint btnwid = 56, btnhei = 42;
	static constexpr uint btn1x = pgx-20-btnwid;
	static constexpr uint btny = panelcy-btnhei/2;
	
	static constexpr uint per_row = 20, row_per_page = 13, per_page = per_row*row_per_page;
	
	int _rect_col(uint ind) const;
	int _rect_row(uint ind) const;
	int _rect_row_pg(uint ind) const;
	int _rect_ind(uint col, uint row) const;
	
	void draw_page_window();
	void draw_grid();
	void draw_info();
	void gui_redraw(bool force_cursor = false);
	void init(optional<int> start_val = nullopt);
	bool is_rectsel() const {return get_cb(OBJPG_CB_RECTSEL);}
	int bound(int v) const {return vbound(v,0,size()-1);};
protected:
	ObjectTemplate() = default;
	uint clk;
	int sel;
	optional<int> sel2;
	string title;
	optional<pair<int,int>> copyind;
	int cb[OBJPG_CB_TOTAL];
	vector<pair<string,std::function<int()>>> buttons;
	
	bool get_cb(uint indx) const;
	void set_cb(uint indx, bool val);
	void toggle_cb(uint indx);
	void write_cb(uint indx);
	
	void show_help() const;
	void try_copy();
	bool try_paste();
	bool try_swap();
	void try_edit();
	bool try_delete();
	bool try_insert();
	bool try_remove();
public:
	void call_dlg(optional<int> start_val = nullopt);
	
	void for_area(int s1, optional<int> s2, std::function<void(int)> callback);
	void for_area(int s1, optional<int> s2, int dest, std::function<void(int,int)> callback);
	void copy_area(int s1, int s2, int dest);
	void draw_null(BITMAP* dest, int x, int y, int w, int h) const;
public:
	//Abstraction
	virtual bool try_adv_paste();
	virtual void do_draw(BITMAP* dest, int x, int y, int w, int h, int index) const = 0;
	virtual void do_copy(int dest, int src) const = 0;
	virtual void do_swap(int dest, int src) const = 0;
	virtual void do_adv_paste(int dest, int src, bitstring const& flags) const {};
	virtual void do_edit(int index) = 0;
	virtual void do_delete(int index) = 0;
	virtual size_t size() const = 0;
	
	virtual bool do_rclick(int indx){return false;}
	virtual bool do_tick(){return false;}
	virtual void postinit(){}
	
	virtual bool disabled_cb(uint indx) const;
	virtual optional<string> cb_get_name(uint indx) const;
	virtual optional<string> cb_get_cfg(uint indx) const;
	virtual optional<bool> cb_get_default(uint indx) const;
	virtual void cb_do_rclick(uint indx);
	
	virtual void clear_backup() const = 0;
	virtual void backup(int index) const = 0;
	virtual void restore_backup() const = 0;
	
	virtual string name() const = 0;
	virtual string cfgname() const = 0;
	virtual string custom_info() const {return "";}
};

#define CMBPG_CB_ANIM OBJPG_CB_NUM_GLOBAL
#define CMBPG_NUM_CB  1
class ComboPageObj : public ObjectTemplate
{
	ComboPageObj() = default;
	static ComboPageObj inst;
public:
	static ComboPageObj& get() {return inst;}
	virtual bool try_adv_paste() override;
	virtual void do_draw(BITMAP* dest, int x, int y, int w, int h, int index) const override;
	virtual void do_copy(int dest, int src) const override;
	virtual void do_swap(int dest, int src) const override;
	virtual void do_adv_paste(int dest, int src, bitstring const& flags) const override;
	virtual void do_edit(int index) override;
	virtual void do_delete(int index) override;
	virtual size_t size() const override;
	
	virtual bool do_rclick(int indx) override;
	virtual bool do_tick() override;
	virtual void postinit() override;
	
	virtual bool disabled_cb(uint indx) const override;
	virtual optional<string> cb_get_name(uint indx) const override;
	virtual optional<string> cb_get_cfg(uint indx) const override;
	virtual optional<bool> cb_get_default(uint indx) const override;
	// virtual void cb_do_rclick(uint indx) override;
	
	virtual void clear_backup() const override;
	virtual void backup(int index) const override;
	virtual void restore_backup() const override;
	
	virtual string name() const override {return "Combo";}
	virtual string cfgname() const override {return "cmb";}
	// virtual string custom_info() const {return "";}
};

#define CPOOLPG_CB_CYCLE OBJPG_CB_NUM_GLOBAL
#define CPOOLPG_NUM_CB 1
class ComboPoolPageObj : public ObjectTemplate
{
	ComboPoolPageObj() = default;
	uint pool_cycle_rate;
	static ComboPoolPageObj inst;
public:
	static ComboPoolPageObj& get() {return inst;}
	
	virtual void do_draw(BITMAP* dest, int x, int y, int w, int h, int index) const override;
	virtual void do_copy(int dest, int src) const override;
	virtual void do_swap(int dest, int src) const override;
	virtual void do_edit(int index) override;
	virtual void do_delete(int index) override;
	virtual size_t size() const override;
	
	virtual bool do_rclick(int indx) override;
	virtual bool do_tick() override;
	virtual void postinit() override;
	
	virtual bool disabled_cb(uint indx) const override;
	virtual optional<string> cb_get_name(uint indx) const override;
	virtual optional<string> cb_get_cfg(uint indx) const override;
	virtual optional<bool> cb_get_default(uint indx) const override;
	virtual void cb_do_rclick(uint indx) override;
	
	virtual void clear_backup() const override;
	virtual void backup(int index) const override;
	virtual void restore_backup() const override;
	
	virtual string name() const override {return "Combo Pool";}
	virtual string cfgname() const override {return "cpool";}
	// virtual string custom_info() const {return "";}
};

class AutoComboPageObj : public ObjectTemplate
{
	AutoComboPageObj() = default;
	static AutoComboPageObj inst;
public:
	static AutoComboPageObj& get() {return inst;}
	virtual void do_draw(BITMAP* dest, int x, int y, int w, int h, int index) const override;
	virtual void do_copy(int dest, int src) const override;
	virtual void do_swap(int dest, int src) const override;
	virtual void do_edit(int index) override;
	virtual void do_delete(int index) override;
	virtual size_t size() const override;
	
	virtual bool do_rclick(int indx) override;
	virtual bool do_tick() override;
	virtual void postinit() override;
	
	// virtual bool disabled_cb(uint indx) const override;
	// virtual optional<string> cb_get_name(uint indx) const override;
	// virtual optional<string> cb_get_cfg(uint indx) const override;
	// virtual optional<bool> cb_get_default(uint indx) const override;
	// virtual void cb_do_rclick(uint indx) override;
	
	virtual void clear_backup() const override;
	virtual void backup(int index) const override;
	virtual void restore_backup() const override;
	
	virtual string name() const override {return "Auto Combo";}
	virtual string cfgname() const override {return "autocmb";}
	// virtual string custom_info() const {return "";}
};

class AliasPageObj : public ObjectTemplate
{
	AliasPageObj() = default;
	static AliasPageObj inst;
public:
	static AliasPageObj& get() {return inst;}
	virtual void do_draw(BITMAP* dest, int x, int y, int w, int h, int index) const override;
	virtual void do_copy(int dest, int src) const override;
	virtual void do_swap(int dest, int src) const override;
	virtual void do_edit(int index) override;
	virtual void do_delete(int index) override;
	virtual size_t size() const override;
	
	virtual bool do_rclick(int indx) override;
	virtual bool do_tick() override;
	virtual void postinit() override;
	
	// virtual bool disabled_cb(uint indx) const override;
	// virtual optional<string> cb_get_name(uint indx) const override;
	// virtual optional<string> cb_get_cfg(uint indx) const override;
	// virtual optional<bool> cb_get_default(uint indx) const override;
	// virtual void cb_do_rclick(uint indx) override;
	
	virtual void clear_backup() const override;
	virtual void backup(int index) const override;
	virtual void restore_backup() const override;
	
	virtual string name() const override {return "Combo Aliases";}
	virtual string cfgname() const override {return "aliases";}
	// virtual string custom_info() const {return "";}
};

#endif

