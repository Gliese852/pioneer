// Copyright © 2008-2020 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "SystemView.h"

#include "AnimationCurves.h"
#include "Frame.h"
#include "Game.h"
#include "GameLog.h"
#include "Lang.h"
#include "Pi.h"
#include "Player.h"
#include "SectorView.h"
#include "Space.h"
#include "StringF.h"
#include "galaxy/Galaxy.h"
#include "galaxy/StarSystem.h"
#include "graphics/Material.h"
#include "graphics/Renderer.h"
#include "graphics/TextureBuilder.h"
#include "lua/LuaObject.h"
#include <iomanip>
#include <sstream>

using namespace Graphics;

const double SystemView::PICK_OBJECT_RECT_SIZE = 12.0;
const Uint16 SystemView::N_VERTICES_MAX = 100;
static const float MIN_ZOOM = 1e-30f; // Just to avoid having 0
static const float MAX_ZOOM = 1e30f;
static const float ZOOM_IN_SPEED = 3;
static const float ZOOM_OUT_SPEED = 3;
static const float WHEEL_SENSITIVITY = .1f; // Should be a variable in user settings.
static const double DEFAULT_VIEW_DISTANCE = 10.0;

TransferPlanner::TransferPlanner() :
	m_position(0., 0., 0.),
	m_velocity(0., 0., 0.)
{
	m_dvPrograde = 0.0;
	m_dvNormal = 0.0;
	m_dvRadial = 0.0;
	m_startTime = 0.0;
	m_factor = 1;
}

vector3d TransferPlanner::GetVel() const { return m_velocity + GetOffsetVel(); }

vector3d TransferPlanner::GetOffsetVel() const
{
	if (m_position.ExactlyEqual(vector3d(0., 0., 0.)))
		return vector3d(0., 0., 0.);

	const vector3d pNormal = m_position.Cross(m_velocity);

	return m_dvPrograde * m_velocity.Normalized() +
		m_dvNormal * pNormal.Normalized() +
		m_dvRadial * m_position.Normalized();
}

void TransferPlanner::AddStartTime(double timeStep)
{
	if (std::fabs(m_startTime) < 1.)
		m_startTime = Pi::game->GetTime();

	m_startTime += m_factor * timeStep;
	double deltaT = m_startTime - Pi::game->GetTime();
	if (deltaT > 0.) {
		FrameId frameId = Frame::GetFrame(Pi::player->GetFrame())->GetNonRotFrame();
		Frame *frame = Frame::GetFrame(frameId);
		Orbit playerOrbit = Orbit::FromBodyState(Pi::player->GetPositionRelTo(frameId), Pi::player->GetVelocityRelTo(frameId), frame->GetSystemBody()->GetMass());

		m_position = playerOrbit.OrbitalPosAtTime(deltaT);
		m_velocity = playerOrbit.OrbitalVelocityAtTime(frame->GetSystemBody()->GetMass(), deltaT);
	} else
		ResetStartTime();
}

void TransferPlanner::ResetStartTime()
{
	m_startTime = 0;
	Frame *frame = Frame::GetFrame(Pi::player->GetFrame());
	if (!frame || GetOffsetVel().ExactlyEqual(vector3d(0., 0., 0.))) {
		m_position = vector3d(0., 0., 0.);
		m_velocity = vector3d(0., 0., 0.);
	} else {
		frame = Frame::GetFrame(frame->GetNonRotFrame());
		m_position = Pi::player->GetPositionRelTo(frame->GetId());
		m_velocity = Pi::player->GetVelocityRelTo(frame->GetId());
	}
}

double TransferPlanner::GetStartTime() const
{
	return m_startTime < 0.0 ? 0.0 : m_startTime;
}

static std::string formatTime(double t)
{
	std::stringstream formattedTime;
	formattedTime << std::setprecision(1) << std::fixed;
	double absT = fabs(t);
	if (absT < 60.)
		formattedTime << t << "s";
	else if (absT < 3600)
		formattedTime << t / 60. << "m";
	else if (absT < 86400)
		formattedTime << t / 3600. << "h";
	else if (absT < 31536000)
		formattedTime << t / 86400. << "d";
	else
		formattedTime << t / 31536000. << "y";
	return formattedTime.str();
}

std::string TransferPlanner::printDeltaTime()
{
	std::stringstream out;
	out << std::setw(9);
	double deltaT = m_startTime - Pi::game->GetTime();
	if (std::fabs(m_startTime) < 1.)
		out << Lang::NOW;
	else
		out << formatTime(deltaT);

	return out.str();
}

void TransferPlanner::AddDv(BurnDirection d, double dv)
{
	if (m_position.ExactlyEqual(vector3d(0., 0., 0.))) {
		FrameId frame = Frame::GetFrame(Pi::player->GetFrame())->GetNonRotFrame();
		m_position = Pi::player->GetPositionRelTo(frame);
		m_velocity = Pi::player->GetVelocityRelTo(frame);
		m_startTime = Pi::game->GetTime();
	}

	switch (d) {
	case PROGRADE: m_dvPrograde += m_factor * dv; break;
	case NORMAL: m_dvNormal += m_factor * dv; break;
	case RADIAL: m_dvRadial += m_factor * dv; break;
	}
}

void TransferPlanner::ResetDv(BurnDirection d)
{
	switch (d) {
	case PROGRADE: m_dvPrograde = 0; break;
	case NORMAL: m_dvNormal = 0; break;
	case RADIAL: m_dvRadial = 0; break;
	}

	if (std::fabs(m_startTime) < 1. &&
		GetOffsetVel().ExactlyEqual(vector3d(0., 0., 0.))) {
		m_position = vector3d(0., 0., 0.);
		m_velocity = vector3d(0., 0., 0.);
		m_startTime = 0.;
	}
}

void TransferPlanner::ResetDv()
{
	m_dvPrograde = 0;
	m_dvNormal = 0;
	m_dvRadial = 0;

	if (std::fabs(m_startTime) < 1.) {
		m_position = vector3d(0., 0., 0.);
		m_velocity = vector3d(0., 0., 0.);
		m_startTime = 0.;
	}
}

double TransferPlanner::GetDv(BurnDirection d)
{
	switch (d) {
	case PROGRADE: return m_dvPrograde; break;
	case NORMAL: return m_dvNormal; break;
	case RADIAL: return m_dvRadial; break;
	}
	return 0.0;
}

std::string TransferPlanner::printDv(BurnDirection d)
{
	double dv = 0;
	char buf[10];

	switch (d) {
	case PROGRADE: dv = m_dvPrograde; break;
	case NORMAL: dv = m_dvNormal; break;
	case RADIAL: dv = m_dvRadial; break;
	}

	snprintf(buf, sizeof(buf), "%6.0fm/s", dv);
	return std::string(buf);
}

void TransferPlanner::IncreaseFactor(void)
{
	if (m_factor > 1000) return;
	m_factor *= m_factorFactor;
}
void TransferPlanner::ResetFactor(void) { m_factor = 1; }

void TransferPlanner::DecreaseFactor(void)
{
	if (m_factor < 0.0002) return;
	m_factor /= m_factorFactor;
}

std::string TransferPlanner::printFactor(void)
{
	char buf[16];
	snprintf(buf, sizeof(buf), "%8gx", 10 * m_factor);
	return std::string(buf);
}

vector3d TransferPlanner::GetPosition() const { return m_position; }

void TransferPlanner::SetPosition(const vector3d &position) { m_position = position; }

SystemView::SystemView(Game *game) :
	UIView(),
	m_game(game),
	m_gridDrawing(GridDrawing::OFF),
	m_shipDrawing(OFF),
	m_showL4L5(LAG_OFF)
{
	SetTransparency(true);

	Graphics::RenderStateDesc rsd;
	m_lineState = Pi::renderer->CreateRenderState(rsd); //m_renderer not set yet

	m_realtime = true;
	m_unexplored = true;

	Gui::Screen::PushFont("OverlayFont");
	m_objectLabels = new Gui::LabelSet();
	Add(m_objectLabels, 0, 0);

	Gui::Screen::PopFont();

	m_infoLabel = (new Gui::Label(""))->Color(178, 178, 178);
	Add(m_infoLabel, 2, 0);

	m_onMouseWheelCon =
		Pi::input.onMouseWheel.connect(sigc::mem_fun(this, &SystemView::MouseWheel));

	Graphics::TextureBuilder b1 = Graphics::TextureBuilder::UI("icons/periapsis.png");
	m_periapsisIcon.reset(new Gui::TexturedQuad(b1.GetOrCreateTexture(Gui::Screen::GetRenderer(), "ui")));
	Graphics::TextureBuilder b2 = Graphics::TextureBuilder::UI("icons/apoapsis.png");
	m_apoapsisIcon.reset(new Gui::TexturedQuad(b2.GetOrCreateTexture(Gui::Screen::GetRenderer(), "ui")));

	Graphics::TextureBuilder l4 = Graphics::TextureBuilder::UI("icons/l4.png");
	m_l4Icon.reset(new Gui::TexturedQuad(l4.GetOrCreateTexture(Gui::Screen::GetRenderer(), "ui")));
	Graphics::TextureBuilder l5 = Graphics::TextureBuilder::UI("icons/l5.png");
	m_l5Icon.reset(new Gui::TexturedQuad(l5.GetOrCreateTexture(Gui::Screen::GetRenderer(), "ui")));

	ResetViewpoint();

	RefreshShips();
	m_planner = Pi::planner;

	m_orbitVts.reset(new vector3f[N_VERTICES_MAX]);
	m_orbitColors.reset(new Color[N_VERTICES_MAX]);
}

SystemView::~SystemView()
{
	m_contacts.clear();
	m_onMouseWheelCon.disconnect();
}

void SystemView::OnClickAccel(float step)
{
	m_realtime = false;
	m_timeStep = step;
}

void SystemView::OnClickRealt()
{
	m_realtime = true;
}

void SystemView::ResetViewpoint()
{
	m_selectedObject.type = Projectable::NONE;
	m_rot_y = 0;
	m_rot_x = 50;
	m_rot_y_to = m_rot_y;
	m_rot_x_to = m_rot_x;
	m_zoom = 1.0f / float(AU);
	m_zoomTo = m_zoom;
	m_timeStep = 1.0f;
	m_time = m_game->GetTime();
}

void SystemView::PutOrbit(const Orbit *orbit, const vector3d &offset, const Color &color, const double planetRadius, const bool showLagrange)
{
	double maxT = 1.;
	unsigned short num_vertices = 0;
	for (unsigned short i = 0; i < N_VERTICES_MAX; ++i) {
		const double t = double(i) / double(N_VERTICES_MAX);
		const vector3d pos = orbit->EvenSpacedPosTrajectory(t);
		if (pos.Length() < planetRadius) {
			maxT = t;
			break;
		}
	}

	static const float startTrailPercent = 0.85;
	static const float fadedColorParameter = 0.8;

	Uint16 fadingColors = 0;
	const double tMinust0 = m_time - m_game->GetTime();
	for (unsigned short i = 0; i < N_VERTICES_MAX; ++i) {
		const double t = double(i) / double(N_VERTICES_MAX) * maxT;
		if (fadingColors == 0 && t >= startTrailPercent * maxT)
			fadingColors = i;
		const vector3d pos = orbit->EvenSpacedPosTrajectory(t, tMinust0);
		m_orbitVts[i] = vector3f(offset + pos * double(m_zoom));
		++num_vertices;
		if (pos.Length() < planetRadius)
			break;
	}

	const Color fadedColor = color * fadedColorParameter;
	std::fill_n(m_orbitColors.get(), num_vertices, fadedColor);
	const Uint16 trailLength = num_vertices - fadingColors;

	for (Uint16 currentColor = 0; currentColor < trailLength; ++currentColor) {
		float scalingParameter = fadedColorParameter + static_cast<float>(currentColor) / trailLength * (1.f - fadedColorParameter);
		m_orbitColors[currentColor + fadingColors] = color * scalingParameter;
	}

	if (num_vertices > 1) {
		m_orbits.SetData(num_vertices, m_orbitVts.get(), m_orbitColors.get());

		// don't close the loop for hyperbolas and parabolas and crashed ellipses
		if (maxT < 1. || (orbit->GetEccentricity() > 1.0)) {
			m_orbits.Draw(m_renderer, m_lineState, LINE_STRIP);
		} else {
			m_orbits.Draw(m_renderer, m_lineState, LINE_LOOP);
		}
	}

	Gui::Screen::EnterOrtho();
	vector3d pos;
	if (Gui::Screen::Project(offset + orbit->Perigeum() * double(m_zoom), pos) && pos.z < 1)
		m_periapsisIcon->Draw(Pi::renderer, vector2f(pos.x - 3, pos.y - 5), vector2f(6, 10), color);
	if (Gui::Screen::Project(offset + orbit->Apogeum() * double(m_zoom), pos) && pos.z < 1)
		m_apoapsisIcon->Draw(Pi::renderer, vector2f(pos.x - 3, pos.y - 5), vector2f(6, 10), color);

	if (showLagrange && m_showL4L5 != LAG_OFF) {
		const Color LPointColor(0x00d6e2ff);
		const vector3d posL4 = orbit->EvenSpacedPosTrajectory((1.0 / 360.0) * 60.0, tMinust0);
		if (Gui::Screen::Project(offset + posL4 * double(m_zoom), pos) && pos.z < 1) {
			m_l4Icon->Draw(Pi::renderer, vector2f(pos.x - 2, pos.y - 2), vector2f(4, 4), LPointColor);
			if (m_showL4L5 == LAG_ICONTEXT)
				m_objectLabels->Add(std::string("L4"), sigc::mem_fun(this, &SystemView::OnClickLagrange), pos.x, pos.y);
		}

		const vector3d posL5 = orbit->EvenSpacedPosTrajectory((1.0 / 360.0) * 300.0, tMinust0);
		if (Gui::Screen::Project(offset + posL5 * double(m_zoom), pos) && pos.z < 1) {
			m_l5Icon->Draw(Pi::renderer, vector2f(pos.x - 2, pos.y - 2), vector2f(4, 4), LPointColor);
			if (m_showL4L5 == LAG_ICONTEXT)
				m_objectLabels->Add(std::string("L5"), sigc::mem_fun(this, &SystemView::OnClickLagrange), pos.x, pos.y);
		}
	}
	Gui::Screen::LeaveOrtho();
}

void SystemView::OnClickLagrange()
{
}

void SystemView::PutLabel(const SystemBody *b, const vector3d &offset)
{
	Gui::Screen::EnterOrtho();

	vector3d pos;
	if (Gui::Screen::Project(offset, pos) && pos.z < 1) {
		AddProjected(Projectable(Projectable::OBJECT, b, pos));
	}
	Gui::Screen::LeaveOrtho();
}

void SystemView::LabelShip(Ship *s, const vector3d &offset)
{
	Gui::Screen::EnterOrtho();

	vector3d pos;
	if (Gui::Screen::Project(offset, pos) && pos.z < 1) {
		AddProjected(Projectable(Projectable::OBJECT, static_cast<Body *>(s), pos));
	}

	Gui::Screen::LeaveOrtho();
}

void SystemView::PutBody(const SystemBody *b, const vector3d &offset, const matrix4x4f &trans)
{
	if (b->GetType() == SystemBody::TYPE_STARPORT_SURFACE)
		return;

	if (b->GetType() != SystemBody::TYPE_GRAVPOINT) {
		if (!m_bodyIcon) {
			Graphics::RenderStateDesc rsd;
			auto solidState = m_renderer->CreateRenderState(rsd);
			m_bodyIcon.reset(new Graphics::Drawables::Disk(m_renderer, solidState, Color::GRAY, 1.0f));
		}

		const double radius = b->GetRadius() * m_zoom;

		matrix4x4f invRot = trans;
		invRot.ClearToRotOnly();
		invRot = invRot.Inverse();

		matrix4x4f bodyTrans = trans;
		bodyTrans.Translate(vector3f(offset));
		bodyTrans.Scale(radius);
		m_renderer->SetTransform(bodyTrans * invRot);
		m_bodyIcon->Draw(m_renderer);

		m_renderer->SetTransform(trans);

		PutLabel(b, offset);
	}

	Frame *frame = Frame::GetFrame(Pi::player->GetFrame());
	if (frame->IsRotFrame())
		frame = Frame::GetFrame(frame->GetNonRotFrame());

	// display the players orbit(?)
	if (frame->GetSystemBody() == b && frame->GetSystemBody()->GetMass() > 0) {
		const double t0 = m_game->GetTime();
		if (Pi::player->IsDocked()) {
			if (m_time == t0) PutSelectionBox(offset + Pi::player->GetPositionRelTo(frame->GetId()) * static_cast<double>(m_zoom), Color::RED);
		} else {
			Orbit playerOrbit = Pi::player->ComputeOrbit();

			PutOrbit(&playerOrbit, offset, Color::RED, b->GetRadius());

			const double plannerStartTime = m_planner->GetStartTime();
			if (!m_planner->GetPosition().ExactlyEqual(vector3d(0, 0, 0))) {
				Orbit plannedOrbit = Orbit::FromBodyState(m_planner->GetPosition(),
					m_planner->GetVel(),
					frame->GetSystemBody()->GetMass());
				PutOrbit(&plannedOrbit, offset, Color::STEELBLUE, b->GetRadius());
				if (std::fabs(m_time - t0) > 1. && (m_time - plannerStartTime) > 0.)
					PutSelectionBox(offset + plannedOrbit.OrbitalPosAtTime(m_time - plannerStartTime) * static_cast<double>(m_zoom), Color::STEELBLUE);
				else
					PutSelectionBox(offset + m_planner->GetPosition() * static_cast<double>(m_zoom), Color::STEELBLUE);
			}

			PutSelectionBox(offset + playerOrbit.OrbitalPosAtTime(m_time - t0) * double(m_zoom), Color::RED);
		}
	}

	// display all child bodies and their orbits
	if (b->HasChildren()) {
		for (const SystemBody *kid : b->GetChildren()) {
			if (is_zero_general(kid->GetOrbit().GetSemiMajorAxis()))
				continue;

			const double axisZoom = kid->GetOrbit().GetSemiMajorAxis() * m_zoom;
			if (axisZoom < DEFAULT_VIEW_DISTANCE) {
				const SystemBody::BodySuperType bst = kid->GetSuperType();
				const bool showLagrange = (bst == SystemBody::SUPERTYPE_ROCKY_PLANET || bst == SystemBody::SUPERTYPE_GAS_GIANT);
				PutOrbit(&(kid->GetOrbit()), offset, Color::GREEN, 0.0, showLagrange);
			}

			// not using current time yet
			const vector3d pos = kid->GetOrbit().OrbitalPosAtTime(m_time) * double(m_zoom);
			PutBody(kid, offset + pos, trans);
		}
	}
}

void SystemView::PutSelectionBox(const SystemBody *b, const vector3d &rootPos, const Color &col)
{
	// surface starports just show the planet as being selected,
	// because SystemView doesn't render terrains anyway
	if (b->GetType() == SystemBody::TYPE_STARPORT_SURFACE)
		b = b->GetParent();
	assert(b);

	vector3d pos = rootPos;
	// while (b->parent), not while (b) because the root SystemBody is defined to be at (0,0,0)
	while (b->GetParent()) {
		pos += b->GetOrbit().OrbitalPosAtTime(m_time) * double(m_zoom);
		b = b->GetParent();
	}

	PutSelectionBox(pos, col);
}

void SystemView::PutSelectionBox(const vector3d &worldPos, const Color &col)
{
	Gui::Screen::EnterOrtho();

	vector3d screenPos;
	if (Gui::Screen::Project(worldPos, screenPos) && screenPos.z < 1) {
		// XXX copied from WorldView::DrawTargetSquare -- these should be unified
		const float x1 = float(screenPos.x - SystemView::PICK_OBJECT_RECT_SIZE * 0.5);
		const float x2 = float(x1 + SystemView::PICK_OBJECT_RECT_SIZE);
		const float y1 = float(screenPos.y - SystemView::PICK_OBJECT_RECT_SIZE * 0.5);
		const float y2 = float(y1 + SystemView::PICK_OBJECT_RECT_SIZE);

		const vector3f verts[4] = {
			vector3f(x1, y1, 0.f),
			vector3f(x2, y1, 0.f),
			vector3f(x2, y2, 0.f),
			vector3f(x1, y2, 0.f)
		};
		m_selectBox.SetData(4, &verts[0], col);
		m_selectBox.Draw(m_renderer, m_lineState, Graphics::LINE_LOOP);
	}

	Gui::Screen::LeaveOrtho();
}


void SystemView::GetTransformTo(const SystemBody *b, vector3d &pos)
{
	if (b->GetParent()) {
		GetTransformTo(b->GetParent(), pos);
		pos -= double(m_zoom) * b->GetOrbit().OrbitalPosAtTime(m_time);
	}
}

void SystemView::GetTransformTo(Projectable p, vector3d &pos)
{
	pos = vector3d(0, 0, 0);
	if (p.reftype == Projectable::SYSTEMBODY)
		GetTransformTo(p.ref.sbody, pos);
	else if (p.reftype == Projectable::BODY && p.ref.body->GetSystemBody())
		GetTransformTo(p.ref.body->GetSystemBody(), pos);
	else // if not systembody, then 100% dynamic body?
	{
		const Body* b = p.ref.body;
		FrameId rootFrameId = m_game->GetSpace()->GetRootFrame();
		FrameId bodyFrameId = b->GetFrame();
		if (b->GetType() == Object::Type::SHIP
				&& static_cast<const Ship*>(b)->GetFlightState() != Ship::FlightState::FLYING)
			pos -= b->GetPositionRelTo(rootFrameId);
		else
		{
			if (bodyFrameId != rootFrameId)
				pos -= Frame::GetFrame(bodyFrameId)->GetPositionRelTo(rootFrameId);
			pos -= static_cast<const DynamicBody*>(b)->ComputeOrbit().OrbitalPosAtTime(m_time - m_game->GetTime());
		}
		//scaling to camera scale
		pos *= double(m_zoom);
	}
}

void SystemView::Draw3D()
{
	PROFILE_SCOPED()
	m_renderer->SetPerspectiveProjection(m_camera_fov, m_renderer->GetDisplayAspect(), 1.f, 1000.f * m_zoom * float(AU) + DEFAULT_VIEW_DISTANCE * 2);
	m_renderer->ClearScreen();
	m_projected.clear();
	//TODO add reserve

	SystemPath path = m_game->GetSectorView()->GetSelected().SystemOnly();
	if (m_system) {
		if (m_system->GetUnexplored() != m_unexplored || !m_system->GetPath().IsSameSystem(path)) {
			m_system.Reset();
			ResetViewpoint();
		}
	}


	if (m_realtime) {
		m_time = m_game->GetTime();
	} else {
		m_time += m_timeStep * Pi::GetFrameTime();
	}
	std::string t = Lang::TIME_POINT + format_date(m_time);

	if (!m_system) {
		m_system = m_game->GetGalaxy()->GetStarSystem(path);
		m_unexplored = m_system->GetUnexplored();
	}

	matrix4x4f trans = matrix4x4f::Identity();
	trans = matrix4x4f::Identity();
	trans.Translate(0, 0, -DEFAULT_VIEW_DISTANCE);
	trans.Rotate(DEG2RAD(m_rot_x), 1, 0, 0);
	trans.Rotate(DEG2RAD(m_rot_y), 0, 1, 0);
	m_renderer->SetTransform(trans);

	vector3d pos(0, 0, 0);
	if (m_selectedObject.type != Projectable::NONE) GetTransformTo(m_selectedObject, pos);

	// glLineWidth(2);
	m_objectLabels->Clear();
	if (m_system->GetUnexplored())
		m_infoLabel->SetText(Lang::UNEXPLORED_SYSTEM_NO_SYSTEM_VIEW);
	else {
		if (m_system->GetRootBody()) {
			PutBody(m_system->GetRootBody().Get(), pos, trans);
		}
	}
	// glLineWidth(1);

	if (m_shipDrawing != OFF) {
		RefreshShips();
		DrawShips(m_time - m_game->GetTime(), pos);
	}

	if (m_gridDrawing != GridDrawing::OFF) {
		DrawGrid();
	}

	UIView::Draw3D();
}

void SystemView::Update()
{
	const float ft = Pi::GetFrameTime();
	// TODO: add "true" lower/upper bounds to m_zoomTo / m_zoom
	m_zoomTo = Clamp(m_zoomTo, MIN_ZOOM, MAX_ZOOM);
	m_zoom = Clamp(m_zoom, MIN_ZOOM, MAX_ZOOM);
	// Since m_zoom changes over multiple orders of magnitude, any fixed linear factor will not be appropriate
	// at some of them.
	AnimationCurves::Approach(m_zoom, m_zoomTo, ft, 10.f, m_zoomTo / 60.f);

	AnimationCurves::Approach(m_rot_x, m_rot_x_to, ft);
	AnimationCurves::Approach(m_rot_y, m_rot_y_to, ft);

	if (Pi::input.MouseButtonState(SDL_BUTTON_MIDDLE)) {
		int motion[2];
		Pi::input.GetMouseMotion(motion);
		m_rot_x_to += motion[1] * 20 * ft;
		m_rot_y_to += motion[0] * 20 * ft;
	}

	UIView::Update();
}

void SystemView::MouseWheel(bool up)
{
	if (this == Pi::GetView()) {
		if (!up)
			m_zoomTo *= 1 / ((ZOOM_OUT_SPEED - 1) * WHEEL_SENSITIVITY + 1) / Pi::GetMoveSpeedShiftModifier();
		else
			m_zoomTo *= ((ZOOM_IN_SPEED - 1) * WHEEL_SENSITIVITY + 1) * Pi::GetMoveSpeedShiftModifier();
	}
}

void SystemView::RefreshShips(void)
{
	m_contacts.clear();
	if (!m_game->GetSpace()->GetStarSystem()->GetPath().IsSameSystem(m_game->GetSectorView()->GetSelected()))
		return;

	auto bs = m_game->GetSpace()->GetBodies();
	for (auto s = bs.begin(); s != bs.end(); s++) {
		if ((*s) != Pi::player &&
			(*s)->GetType() == Object::SHIP) {

			const auto c = static_cast<Ship *>(*s);
			m_contacts.push_back(std::make_pair(c, c->ComputeOrbit()));
		}
	}
}

void SystemView::DrawShips(const double t, const vector3d &offset)
{
	for (auto s = m_contacts.begin(); s != m_contacts.end(); s++) {
		vector3d pos = offset;
		if ((*s).first->GetFlightState() != Ship::FlightState::FLYING) {
			FrameId frameId = m_game->GetSpace()->GetRootFrame();
			pos += (*s).first->GetPositionRelTo(frameId) * double(m_zoom);
		} else {
			FrameId frameId = (*s).first->GetFrame();
			vector3d bpos = vector3d(0., 0., 0.);
			if (frameId != m_game->GetSpace()->GetRootFrame()) {
				Frame *frame = Frame::GetFrame(frameId);
				bpos += frame->GetPositionRelTo(m_game->GetSpace()->GetRootFrame());
			}
			pos += (bpos + (*s).second.OrbitalPosAtTime(t)) * double(m_zoom);
		}
		const bool isSelected = m_selectedObject.reftype == Projectable::BODY && m_selectedObject.ref.body == (*s).first;
		LabelShip((*s).first, pos);

		if (m_shipDrawing == ORBITS && (*s).first->GetFlightState() == Ship::FlightState::FLYING)
			PutOrbit(&(*s).second, offset, isSelected ? Color::GREEN : Color::BLUE, 0);
	}
}

void SystemView::PrepareGrid()
{
	// calculate lines for this system:
	double diameter = std::floor(m_system->GetRootBody()->GetMaxChildOrbitalDistance() * 1.2 / AU);

	m_grid_lines = int(diameter) + 1;

	m_displayed_sbody.clear();
	if (m_gridDrawing == GridDrawing::GRID_AND_LEGS) {
		m_displayed_sbody = m_system->GetRootBody()->CollectAllChildren();
	}
}

void SystemView::DrawGrid()
{
	PrepareGrid();

	m_lineVerts.reset(new Graphics::VertexArray(Graphics::ATTRIB_POSITION, m_grid_lines * 4 + m_displayed_sbody.size() * 2));

	float zoom = m_zoom * float(AU);
	vector3d pos(0.);
	if (m_selectedObject.type != Projectable::NONE) GetTransformTo(m_selectedObject, pos);

	for (int i = -m_grid_lines; i < m_grid_lines + 1; i++) {
		float z = float(i) * zoom;
		m_lineVerts->Add(vector3f(-m_grid_lines * zoom, 0.0f, z) + vector3f(pos), Color::GRAY);
		m_lineVerts->Add(vector3f(+m_grid_lines * zoom, 0.0f, z) + vector3f(pos), Color::GRAY);
	}

	for (int i = -m_grid_lines; i < m_grid_lines + 1; i++) {
		float x = float(i) * zoom;
		m_lineVerts->Add(vector3f(x, 0.0f, -m_grid_lines * zoom) + vector3f(pos), Color::GRAY);
		m_lineVerts->Add(vector3f(x, 0.0f, +m_grid_lines * zoom) + vector3f(pos), Color::GRAY);
	}

	for (SystemBody *sbody : m_displayed_sbody) {
		vector3d offset(0.);
		GetTransformTo(sbody, offset);
		m_lineVerts->Add(vector3f(pos - offset), Color::GRAY * 0.5);
		offset.y = 0.0;
		m_lineVerts->Add(vector3f(pos - offset), Color::GRAY * 0.5);
	}

	m_lines.SetData(m_lineVerts->GetNumVerts(), &m_lineVerts->position[0], &m_lineVerts->diffuse[0]);
	m_lines.Draw(Pi::renderer, m_lineState);
}

void SystemView::AddProjected(Projectable p)
{
	float scale[2];
	Gui::Screen::GetCoords2Pixels(scale);
	p.screenpos.x = p.screenpos.x / scale[0];
	p.screenpos.y = p.screenpos.y / scale[1];
	m_projected.push_back(p);
}

void SystemView::CenterOn(Projectable p)
{
	//if (m_selectedObject.reftype == Projectable::BODY && m_selectedObject.ref.body == b) return true;
	//m_selectedObject.reftype = Projectable::BODY; m_selectedObject.ref.body = b; return false;
	m_selectedObject.type = p.type;
	m_selectedObject.reftype = p.reftype;
	if (p.reftype == Projectable::BODY) m_selectedObject.ref.body = p.ref.body;
	else m_selectedObject.ref.sbody = p.ref.sbody;
	Output("selected: %d, %d", m_selectedObject.type, m_selectedObject.reftype);
}

void SystemView::BodyInaccessible(Body *b)
{
	if (m_selectedObject.reftype == Projectable::BODY && m_selectedObject.ref.body == b) m_selectedObject.type = Projectable::NONE;
}

void SystemView::SetVisibility(std::string param)
{
	if (param == "RESET_VIEW") ResetViewpoint();
	else if (param == "GRID_OFF") m_gridDrawing = GridDrawing::OFF;
	else if (param == "GRID_ON") m_gridDrawing = GridDrawing::GRID;
	else if (param == "GRID_AND_LEGS") m_gridDrawing = GridDrawing::GRID_AND_LEGS;
	else if (param == "LAG_OFF") m_showL4L5 = LAG_OFF;
	else if (param == "LAG_ICON") m_showL4L5 = LAG_ICON;
	else if (param == "LAG_ICONTEXT") m_showL4L5 = LAG_ICONTEXT;
	else if (param == "SHIPS_OFF") m_shipDrawing = OFF;
	else if (param == "SHIPS_ON") m_shipDrawing = BOXES;
	else if (param == "SHIPS_ORBITS") m_shipDrawing = ORBITS;
	else if (param == "ZOOM_IN")
		m_zoomTo *= pow(ZOOM_IN_SPEED * Pi::GetMoveSpeedShiftModifier(), Pi::GetFrameTime());
	else if (param == "ZOOM_OUT")
		m_zoomTo *= pow(ZOOM_OUT_SPEED * Pi::GetMoveSpeedShiftModifier(), Pi::GetFrameTime());
	else Output("Unknown visibility: %s\n", param.c_str());
}

Projectable SystemView::GetSelectedObject()
{
	return m_selectedObject;
}
