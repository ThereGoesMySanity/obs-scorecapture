#include <obs-module.h>
#include <QPainter>
#include <QLocale>
#include <QImage>

#include "SCSource.hpp"
#include "plugin-support.h"

void MakeQFont(obs_data_t *font_obj, QFont &font)
{
	const char *face = obs_data_get_string(font_obj, "face");
	const char *style = obs_data_get_string(font_obj, "style");
	int size = (int)obs_data_get_int(font_obj, "size");
	uint32_t flags = (uint32_t)obs_data_get_int(font_obj, "flags");

	if (face) {
		font.setFamily(face);
		font.setStyleName(style);
	}

	if (size) {
		font.setPointSize(size);
	}

	if (flags & OBS_FONT_BOLD)
		font.setBold(true);
	if (flags & OBS_FONT_ITALIC)
		font.setItalic(true);
	if (flags & OBS_FONT_UNDERLINE)
		font.setUnderline(true);
	if (flags & OBS_FONT_STRIKEOUT)
		font.setStrikeOut(true);
}


SCSource::SCSource(obs_source_t *_source)
	: source(_source),
	  texrender(gs_texrender_create(GS_BGRA, GS_ZS_NONE)),
	  enableSignal(
		  obs_source_get_signal_handler(source), "enable",
		  [](void *data, calldata_t *cd) {
			  static_cast<SCSource *>(data)->enable(calldata_bool(cd, "enabled"));
		  },
		  this)
{
	img = QImage(width, height, QImage::Format_ARGB32);
	proc_handler_t *ph = obs_source_get_proc_handler(source);
	proc_handler_add(
		ph, "void scores_updated(in ptr p1Score, in ptr p2Score)",
		[](void *data, calldata_t *cd) {
			static_cast<SCSource *>(data)->scoresUpdated(
				*static_cast<std::optional<int> *>(calldata_ptr(cd, "p1Score")),
				*static_cast<std::optional<int> *>(calldata_ptr(cd, "p2Score")));
		},
		this);
	proc_handler_add(
		ph, "void scores_cleared()",
		[](void *data, calldata_t *cd) {
			UNUSED_PARAMETER(cd);
			static_cast<SCSource *>(data)->scoresCleared();
		},
		this);
}

void SCSource::scoresUpdated(std::optional<int> p1Score, std::optional<int> p2Score) {
    this->p1Score = p1Score;
    this->p2Score = p2Score;
    this->rerender = true;
}

void SCSource::scoresCleared() {
	p1Score = {};
	p2Score = {};
	rerender = true;
}

SCSource::~SCSource() {
    obs_enter_graphics();
    gs_texrender_destroy(texrender);
    obs_leave_graphics();
}

void SCSource::update(obs_data_t *settings) {
	p1Color = QColor::fromRgba(0xFF000000 + obs_data_get_int(settings, "p1_color"));
	p2Color = QColor::fromRgba(0xFF000000 + obs_data_get_int(settings, "p2_color"));
	text_color = QColor::fromRgba(0xFF000000 + obs_data_get_int(settings, "text_color"));
	background_color = QColor::fromRgba(obs_data_get_int(settings, "background_color"));

	max_score = obs_data_get_int(settings, "score_types");

	OBSDataAutoRelease f = obs_data_get_obj(settings, "font");
	MakeQFont(f, font);
	MakeQFont(f, bold_font);
	MakeQFont(f, small_font);
	bold_font.setBold(true);

	font.setBold(false);
	font.setPointSizeF(std::max(font.pointSizeF() - 4, 16.0));

	small_font.setBold(false);
	small_font.setPointSizeF(std::max(small_font.pointSizeF() - 20, 12.0));

	rerender = true;
}

void SCSource::activate() {
    obs_log(LOG_INFO, "scorecapture_source_activate");
	isDisabled = false;
}

void SCSource::deactivate() {
    obs_log(LOG_INFO, "scorecapture_source_deactivate");
	isDisabled = true;
}

void SCSource::enable(bool enabled) {
    isDisabled = !enabled;
}

void SCSource::videoRender(gs_effect_t *effect)
{
	if (isDisabled) {
		return;
	}

	if (rerender) {
		rerender = false;
		img.fill(background_color);
		QPainter ctx(&img);

		const int center = width / 2;
		ctx.fillRect(0, 0, center, 10, p1Color);
		ctx.fillRect(center, 0, center, 10, p2Color);

		if (p1Score && p2Score && max_score > 0) {
			int diff = p1Score.value() - p2Score.value();
			int absDiff = abs(diff);
			qreal size = absDiff <= max_score / 25.0 ? 5.0 * center * absDiff / max_score
								     : center * std::sqrt((qreal)absDiff / max_score);
			size = std::min(size, (double)center);
			QLocale locale;
			QString p1Text = locale.toString(p1Score.value());
			QString p2Text = locale.toString(p2Score.value());
			QString diffText = locale.toString(-absDiff);
			ctx.setPen(text_color);
			if (diff != 0) {
				ctx.setFont(small_font);
				if (diff > 0)
				{
					ctx.fillRect(QRectF(center - size, 0, size, 40), p1Color);
					ctx.drawText(QRectF(center + 5, 15, width - center - 5, height - 15),
						     Qt::AlignTop, diffText);
				}
				else
				{
					ctx.fillRect(QRectF(center, 0, size, 40), p2Color);
					ctx.drawText(QRectF(0, 15, width - center - 5, height - 15),
						     Qt::AlignTop | Qt::AlignRight, diffText);
				}
			}
			ctx.setFont(diff > 0 ? bold_font : font);
			ctx.drawText(QRectF(0, 45, width - center - 10, height - 45),
				     Qt::AlignTop | Qt::AlignRight, p1Text);
			ctx.setFont(diff < 0 ? bold_font : font);
			ctx.drawText(QRectF(center + 10, 45, width - center - 10, height - 45), Qt::AlignTop,
				     p2Text);
		}
	}
	const uint8_t *bits = img.bits();
	gs_texture_t *tex = gs_texture_create(img.width(), img.height(), GS_RGBA, 1, &bits, 0);
	const bool previous = gs_framebuffer_srgb_enabled();
	gs_enable_framebuffer_srgb(true);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	gs_eparam_t *const param = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture_srgb(param, tex);

	gs_draw_sprite(tex, 0, img.width(), img.height());

	gs_blend_state_pop();
	gs_texture_destroy(tex);

	gs_enable_framebuffer_srgb(previous);

}
