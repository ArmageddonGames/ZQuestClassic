#include "info.h"
#include <gui/builder.h>
#include <utility>

InfoDialog::InfoDialog(std::string title, std::string text):
	dlgTitle(title),
	dlgText(text)
{}

InfoDialog::InfoDialog(std::string title, std::vector<std::string_view> lines):
	dlgTitle(title),
	dlgText()
{
	size_t size = 0;

	for(auto& line: lines)
		size += line.size();
	size += lines.size()-1;
	dlgText.reserve(size);

	auto remaining = lines.size();
	for(auto& line: lines)
	{
		dlgText += line;
		--remaining;
		if(remaining > 0)
			dlgText += '\n';
	}
}

std::shared_ptr<GUI::Widget> InfoDialog::view()
{
	using namespace GUI::Builder;
	using namespace GUI::Props;

	return Window(
		title = std::move(dlgTitle),
		onEnter = 0,
		onClose = 0,
		Column(
			Label(
				hPadding = 3_em,
				maxLines = 10,
				textAlign = 1,
				text = std::move(dlgText)),
			Button(
				text = "&Close",
				topPadding = 0.5_em,
				onClick = 0,
				focused = true)
		)
	);
}

bool InfoDialog::handleMessage(const GUI::DialogMessage<message>& msg)
{
	return true;
}
