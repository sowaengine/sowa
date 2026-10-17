#include "resource/font.hpp"
#include "ui/ui_system.hpp"
#include <algorithm>

namespace sowa {
namespace {
size_t previous(const std::string &s, size_t p) {
	p = std::min(p, s.size());
	if (p) {
		--p;
	}
	while (p && (static_cast<unsigned char>(s[p]) & 0xc0) == 0x80) {
		--p;
	}
	return p;
}

size_t next(const std::string &s, size_t p) {
	p = std::min(p, s.size());
	if (p < s.size()) {
		++p;
	}
	while (p < s.size() && (static_cast<unsigned char>(s[p]) & 0xc0) == 0x80) {
		++p;
	}
	return p;
}

u32 count(const std::string &s) {
	u32 n = 0;
	for (size_t p = 0; p < s.size(); p = next(s, p)) {
		++n;
	}
	return n;
}

std::string utf8(u32 cp) {
	std::string s;
	if (cp < 32 || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) {
		return s;
	}
	if (cp < 0x80) {
		s += char(cp);
	} else if (cp < 0x800) {
		s += char(0xc0 | (cp >> 6));
		s += char(0x80 | (cp & 63));
	} else if (cp < 0x10000) {
		s += char(0xe0 | (cp >> 12));
		s += char(0x80 | ((cp >> 6) & 63));
		s += char(0x80 | (cp & 63));
	} else {
		s += char(0xf0 | (cp >> 18));
		s += char(0x80 | ((cp >> 12) & 63));
		s += char(0x80 | ((cp >> 6) & 63));
		s += char(0x80 | (cp & 63));
	}
	return s;
}
} // namespace

void UISystem::positionTextCaret(Entity e, Vector2 p) {
	const auto *n = find(e);
	auto *input = e.Get<UITextInput>();
	if (!n || !input) {
		return;
	}
	const f32 size = n->style.fontSize * n->scale;
	const auto *font = fontFor(*n);
	const f32 line = font ? font->LineHeight() * size / (font->Ascent() - font->Descent()) : size;
	const f32 px = p.x - n->rect.x - n->style.padding.x * n->scale + input->scroll.x;
	const int wanted =
		std::max(0, int((p.y - n->rect.y - n->style.padding.y * n->scale + input->scroll.y) /
						std::max(1.f, line)));
	f32 x = 0;
	int row = 0;
	size_t cursor = 0;
	for (; cursor < input->text.size(); cursor = next(input->text, cursor)) {
		const char c = input->text[cursor];
		if (c == '\n') {
			if (row == wanted) {
				break;
			}
			++row;
			x = 0;
			continue;
		}
		u32 codepoint = static_cast<u8>(c);
		if (codepoint >= 128) {
			codepoint = '?';
		}
		if (input->password) {
			codepoint = '*';
		}
		const Glyph *glyph = font ? font->FindGlyph(codepoint) : nullptr;
		const f32 advance =
			glyph ? glyph->advance * size / (font->Ascent() - font->Descent()) : size * .55f;
		if (row == wanted && px < x + advance / 2) {
			break;
		}
		x += advance;
	}
	input->caret = static_cast<u32>(cursor);
}

void UISystem::editText(const InputEvent &event) {
	if (!_focus.IsAlive()) {
		return;
	}
	auto *input = _focus.Get<UITextInput>();
	if (!input) {
		return;
	}
	input->caret = std::min<u32>(input->caret, input->text.size());
	input->selectionAnchor = std::min<u32>(input->selectionAnchor, input->text.size());
	const std::string before = input->text;
	auto eraseSelection = [&]() {
		const auto a = std::min(input->caret, input->selectionAnchor),
				   b = std::max(input->caret, input->selectionAnchor);
		input->text.erase(a, b - a);
		input->caret = input->selectionAnchor = a;
	};
	auto insert = [&](std::string value) {
		if (input->readOnly) {
			return;
		}
		std::erase(value, '\r');
		if (!input->multiline) {
			std::erase(value, '\n');
		}
		eraseSelection();
		if (input->maxLength > 0) {
			const u32 remaining = std::max(0, input->maxLength - int(count(input->text)));
			size_t end = 0;
			for (u32 i = 0; i < remaining && end < value.size(); ++i) {
				end = next(value, end);
			}
			value.resize(end);
		}
		input->text.insert(input->caret, value);
		input->caret += value.size();
		input->selectionAnchor = input->caret;
	};
	if (event.type == InputEventType::Char) {
		insert(utf8(event.character.codepoint));
	} else if (event.type == InputEventType::Key && event.key.action != KeyAction::Release) {
		const Key key = event.key.key;
		const bool shift = (event.key.mods & 1) != 0, command = (event.key.mods & (2 | 8)) != 0;
		if (command && key == Key::A) {
			input->selectionAnchor = 0;
			input->caret = input->text.size();
		} else if (command && (key == Key::C || key == Key::X)) {
			if (!input->password && _writeClipboard) {
				const auto a = std::min(input->caret, input->selectionAnchor),
						   b = std::max(input->caret, input->selectionAnchor);
				_writeClipboard(input->text.substr(a, b - a));
			}
			if (key == Key::X && !input->readOnly && !input->password) {
				eraseSelection();
			}
		} else if (command && key == Key::V) {
			if (_readClipboard) {
				insert(_readClipboard());
			}
		} else if (key == Key::Backspace || key == Key::Delete) {
			if (!input->readOnly) {
				if (input->caret == input->selectionAnchor) {
					input->selectionAnchor = key == Key::Backspace
												 ? previous(input->text, input->caret)
												 : next(input->text, input->caret);
				}
				eraseSelection();
			}
		} else if (key == Key::Enter || key == Key::KeypadEnter) {
			if (input->multiline && !command) {
				insert("\n");
			} else {
				emit(_focus, "submitted", 0, input->text);
				return;
			}
		} else if (key == Key::Left || key == Key::Right || key == Key::Home || key == Key::End ||
				   key == Key::Up || key == Key::Down) {
			size_t caret = input->caret;
			if (key == Key::Left) {
				caret = previous(input->text, caret);
			}
			if (key == Key::Right) {
				caret = next(input->text, caret);
			}
			size_t start = 0;
			if (caret > 0) {
				const auto newline = input->text.rfind('\n', caret - 1);
				if (newline != std::string::npos) {
					start = newline + 1;
				}
			}
			if (key == Key::Home) {
				caret = command ? 0 : start;
			}
			if (key == Key::End) {
				caret = command ? input->text.size() : input->text.find('\n', caret);
				if (caret == std::string::npos) {
					caret = input->text.size();
				}
			}
			if (key == Key::Up && start > 0) {
				const size_t previousEnd = start - 1;
				const size_t p =
					previousEnd == 0 ? std::string::npos : input->text.rfind('\n', previousEnd - 1);
				const size_t previousStart = p == std::string::npos ? 0 : p + 1;
				caret = std::min(previousEnd, previousStart + (caret - start));
			}
			if (key == Key::Down) {
				const size_t end = input->text.find('\n', caret);
				if (end != std::string::npos) {
					const size_t nextEnd = input->text.find('\n', end + 1);
					caret = std::min(nextEnd == std::string::npos ? input->text.size() : nextEnd,
						end + 1 + (caret - start));
				}
			}
			while (caret > 0 && caret < input->text.size() &&
				   (static_cast<u8>(input->text[caret]) & 0xc0) == 0x80) {
				--caret;
			}
			input->caret = caret;
			if (!shift) {
				input->selectionAnchor = caret;
			}
		}
	}
	if (before != input->text) {
		const auto text = input->text;
		emit(_focus, "changed", 0, text);
	}
}

} // namespace sowa
