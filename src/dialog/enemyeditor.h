#ifndef ZC_DIALOG_ENEMYEDITOR_H_
#define ZC_DIALOG_ENEMYEDITOR_H_

#include <gui/dialog.h>
#include <gui/button.h>
#include <gui/checkbox.h>
#include <gui/label.h>
#include <gui/text_field.h>
#include <gui/window.h>
#include <gui/list_data.h>
#include <zq/gui/seltile_swatch.h>
#include <functional>
#include <string_view>
#include <map>

bool call_enemy_editor(int32_t index);

class EnemyEditorDialog : public GUI::Dialog<EnemyEditorDialog>
{
public:
	enum class message {
		OK, CANCEL, CLEAR, DEFAULT, WARNINGS
	};


	std::shared_ptr<GUI::Widget> view() override;
	bool handleMessage(const GUI::DialogMessage<message>& msg);

private:
	EnemyEditorDialog(guydata const& ref, int32_t index);
	EnemyEditorDialog(int32_t index);

	void refreshScript();
	void loadEnemyType();
	void updateWarnings();
	void apply_enemy();

	std::shared_ptr<GUI::Window> window;
	std::shared_ptr<GUI::TextField> tf_initd[8], tf_wpninitd[8];
	std::shared_ptr<GUI::Label> l_initds[8], l_wpninitds[8];
	std::shared_ptr<GUI::Button> ib_initds[8], ib_wpninitds[8];
	std::shared_ptr<GUI::Button> warnbtn;
	std::shared_ptr<GUI::SelTileSwatch> tswatch[4];

	std::shared_ptr<GUI::Label> l_attributes[32];
	std::shared_ptr<GUI::Widget> w_attributes[32];
	std::shared_ptr<GUI::Button> ib_attributes[32];
	std::shared_ptr<GUI::Label> l_bflags[16];
	std::shared_ptr<GUI::Checkbox> cb_bflags[16];
	std::shared_ptr<GUI::Button> ib_bflags[16];

	string h_initd[8], h_wpninitd[8];
	string l_initd[8], l_wpninitd[8];

	string l_attribute[32], l_bflag[16];
	string enemy_name;
	string turnfreqorhaltrate;

	vector<string> warnings;

	int32_t index;
	guydata local_guyref;

	GUI::ListData list_enemies, list_families, list_animations, list_guyscripts, list_ewpnscripts,
		list_sfx, list_counters_nn, list_dropsets, list_sprites, list_eweaptype, list_deftypes;

	std::shared_ptr<GUI::Widget> NumberField(auto* data, auto _min, auto _max, string str);

	std::shared_ptr<GUI::Widget> NameField(string str);

	std::shared_ptr<GUI::Widget> MiscAttribute(int32_t* index);

	std::shared_ptr<GUI::Widget> DropDownField(auto* field, GUI::ListData ls);

	std::shared_ptr<GUI::Widget> MiscFlag(auto* bitstring, auto index);
	
	friend bool call_enemy_editor(int32_t index);
};
#endif