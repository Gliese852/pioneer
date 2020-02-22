// Copyright © 2008-2020 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "SystemView.h"

#include "AnimationCurves.h"
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
static const int MAX_TRANSITION_FRAMES = 40;

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
	m_showL4L5(LAG_OFF),
	m_trans(0.0),
	m_transTo(0.0)
{
	m_rot_y = 0;
	m_rot_x = 50;
	m_zoom = 1.0f / float(AU);
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
	m_rot_y_to = 0;
	m_rot_x_to = 50;
	m_zoomTo = 1.0f / float(AU);
	m_timeStep = 1.0f;
	m_time = m_game->GetTime();
	m_animateTransition = MAX_TRANSITION_FRAMES;
}

template <typename RefType>
void SystemView::PutOrbit(RefType *ref, const Orbit *orbit, const vector3d &offset, const Color &color, const double planetRadius, const bool showLagrange)
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
		AddProjected<RefType>(Projectable::PERIAPSIS, ref, pos);
		//m_periapsisIcon->Draw(Pi::renderer, vector2f(pos.x - 3, pos.y - 5), vector2f(6, 10), color);
	if (Gui::Screen::Project(offset + orbit->Apogeum() * double(m_zoom), pos) && pos.z < 1)
		AddProjected<RefType>(Projectable::APOAPSIS, ref, pos);
		//m_apoapsisIcon->Draw(Pi::renderer, vector2f(pos.x - 3, pos.y - 5), vector2f(6, 10), color);

	if (showLagrange && m_showL4L5 != LAG_OFF) {
		const Color LPointColor(0x00d6e2ff);
		const vector3d posL4 = orbit->EvenSpacedPosTrajectory((1.0 / 360.0) * 60.0, tMinust0);
		if (Gui::Screen::Project(offset + posL4 * double(m_zoom), pos) && pos.z < 1) {
			AddProjected<RefType>(Projectable::L4, ref, pos);
			//m_l4Icon->Draw(Pi::renderer, vector2f(pos.x - 2, pos.y - 2), vector2f(4, 4), LPointColor);
			//if (m_showL4L5 == LAG_ICONTEXT)
				//m_objectLabels->Add(std::string("L4"), sigc::mem_fun(this, &SystemView::OnClickLagrange), pos.x, pos.y);
		}

		const vector3d posL5 = orbit->EvenSpacedPosTrajectory((1.0 / 360.0) * 300.0, tMinust0);
		if (Gui::Screen::Project(offset + posL5 * double(m_zoom), pos) && pos.z < 1) {
			AddProjected<RefType>(Projectable::L5, ref, pos);
			//m_l5Icon->Draw(Pi::renderer, vector2f(pos.x - 2, pos.y - 2), vector2f(4, 4), LPointColor);
			//if (m_showL4L5 == LAG_ICONTEXT)
			//	m_objectLabels->Add(std::string("L5"), sigc::mem_fun(this, &SystemView::OnClickLagrange), pos.x, pos.y);
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
		AddProjected<const SystemBody>(Projectable::OBJECT, b, pos);
	}
	Gui::Screen::LeaveOrtho();
}

void SystemView::LabelShip(Ship *s, const vector3d &offset)
{
	Gui::Screen::EnterOrtho();

	vector3d pos;
	if (Gui::Screen::Project(offset, pos) && pos.z < 1) {
		AddProjected<Body>(Projectable::OBJECT, static_cast<Body *>(s), pos);
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


	// display all child bodies and their orbits
	if (b->HasChildren()) {
		for (const SystemBody *kid : b->GetChildren()) {
			if (is_zero_general(kid->GetOrbit().GetSemiMajorAxis()))
				continue;

			const double axisZoom = kid->GetOrbit().GetSemiMajorAxis() * m_zoom;
			//if (axisZoom < DEFAULT_VIEW_DISTANCE) {
				const SystemBody::BodySuperType bst = kid->GetSuperType();
				const bool showLagrange = (bst == SystemBody::SUPERTYPE_ROCKY_PLANET || bst == SystemBody::SUPERTYPE_GAS_GIANT);
				PutOrbit<const SystemBody>(kid, &(kid->GetOrbit()), offset, Color::GREEN, 0.0, showLagrange);
			//}

			// not using current time yet
			const vector3d pos = kid->GetOrbit().OrbitalPosAtTime(m_time) * double(m_zoom);
			PutBody(kid, offset + pos, trans);
		}
	}

	// display the players orbit(?)
	/*
	if (frame->GetSystemBody() == b && frame->GetSystemBody()->GetMass() > 0) {
		Body* PlayerBody = static_cast<Body*>(Pi::player);
		const double t0 = m_game->GetTime();
		vector3d pos(0.0);
		Orbit playerOrbit = Pi::player->ComputeOrbit();
		//CalculateShipPositionAtTime(static_cast<Ship*>(Pi::player), playerOrbit, m_time, pos);
		//AddNotProjected<Body>(Projectable::PLAYERSHIP, PlayerBody, pos * static_cast<double>(m_zoom));
		PutOrbit<Body>(PlayerBody, &playerOrbit, offset, Color::RED, b->GetRadius());

		const double plannerStartTime = m_planner->GetStartTime();
		if (!m_planner->GetPosition().ExactlyEqual(vector3d(0, 0, 0))) {
			Orbit plannedOrbit = Orbit::FromBodyState(m_planner->GetPosition(),
					m_planner->GetVel(),
					frame->GetSystemBody()->GetMass());
			PutOrbit<Body>(PlayerBody, &plannedOrbit, offset, Color::STEELBLUE, b->GetRadius());
			if (std::fabs(m_time - t0) > 1. && (m_time - plannerStartTime) > 0.)
				//PutSelectionBox(offset + plannedOrbit.OrbitalPosAtTime(m_time - plannerStartTime) * static_cast<double>(m_zoom), Color::STEELBLUE);
				AddNotProjected<Body>(Projectable::PLANNER, PlayerBody, offset + plannedOrbit.OrbitalPosAtTime(m_time - plannerStartTime) * static_cast<double>(m_zoom));
			else
				//PutSelectionBox(offset + m_planner->GetPosition() * static_cast<double>(m_zoom), Color::STEELBLUE);
				AddNotProjected<Body>(Projectable::PLANNER, PlayerBody, offset + m_planner->GetPosition() * static_cast<double>(m_zoom));
		}

		//PutSelectionBox(offset + playerOrbit.OrbitalPosAtTime(m_time - t0) * double(m_zoom), Color::RED);
		//AddNotProjected<Body>(Projectable::PLAYERSHIP, PlayerBody, offset + playerOrbit.OrbitalPosAtTime(m_time - t0) * double(m_zoom));

	
	}
	*/
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
		pos -= b->GetOrbit().OrbitalPosAtTime(m_time);
	}
}

void SystemView::GetTransformTo(Projectable &p, vector3d &pos)
{
	pos = vector3d(0., 0., 0.);
	if (p.reftype == Projectable::SYSTEMBODY)
		GetTransformTo(p.ref.sbody, pos);
	else if (p.reftype == Projectable::BODY && p.ref.body->GetSystemBody())
		GetTransformTo(p.ref.body->GetSystemBody(), pos);
	else if (p.ref.body->GetType() == Object::Type::SHIP or p.ref.body->GetType() == Object::Type::PLAYER)
	{
		const Ship* s = static_cast<const Ship*>(p.ref.body);
		CalculateShipPositionAtTime(s, s->ComputeOrbit(), m_time, pos);
		pos = -pos;
	}
}

void SystemView::CalculateShipPositionAtTime(const Ship *s, Orbit o, double t, vector3d &pos)
{
	pos = vector3d(0., 0., 0.);
	FrameId shipFrameId = s->GetFrame();
	FrameId shipNonRotFrameId = Frame::GetFrame(shipFrameId)->GetNonRotFrame();
	if (s->GetFlightState() != Ship::FlightState::FLYING) {
		vector3d rpos(0.0);
		if (Frame::GetFrame(shipFrameId)->IsRotFrame())
		{
			Frame* rotframe = Frame::GetFrame(shipFrameId);
			if (t == m_game->GetTime())
			{
				pos = s->GetPositionRelTo(m_game->GetSpace()->GetRootFrame());
				return;
			}
			else
				rpos = s->GetPositionRelTo(shipNonRotFrameId) * rotframe->GetOrient() * matrix3x3d::RotateY(rotframe->GetAngSpeed() * (t - m_game->GetTime())) * rotframe->GetOrient().Transpose();
		}
		vector3d fpos(0.0);
		CalculateFramePositionAtTime(shipNonRotFrameId, t, fpos);
		pos += fpos + rpos;
	} else {
		vector3d fpos(0.0);
		CalculateFramePositionAtTime(shipNonRotFrameId, t, fpos);
		pos += (fpos + o.OrbitalPosAtTime(t - m_game->GetTime()));
	}
}

//frame must be nonrot
void SystemView::CalculateFramePositionAtTime(FrameId frameId, double t, vector3d &pos)
{
	if (frameId == m_game->GetSpace()->GetRootFrame()) pos = vector3d(0., 0., 0.);
	else
	{
		Frame *frame = Frame::GetFrame(frameId);
		CalculateFramePositionAtTime(frame->GetParent(), t, pos);
		pos += frame->GetSystemBody()->GetOrbit().OrbitalPosAtTime(t);
	}
}

void SystemView::Draw3D()
{
	PROFILE_SCOPED()
	m_renderer->SetPerspectiveProjection(CAMERA_FOV, m_renderer->GetDisplayAspect(), 1.f, 1000.f * m_zoom * float(AU) + DEFAULT_VIEW_DISTANCE * 2);
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

	m_transTo *= 0.0;
	if (m_selectedObject.type != Projectable::NONE) GetTransformTo(m_selectedObject, m_transTo);
	if(m_animateTransition)
	{
		const float ft = Pi::GetFrameTime();
		m_animateTransition--;
		AnimationCurves::Approach(m_trans.x, m_transTo.x, ft);
		AnimationCurves::Approach(m_trans.y, m_transTo.y, ft);
		AnimationCurves::Approach(m_trans.z, m_transTo.z, ft);
	} else {
		m_trans = m_transTo;
	}

	vector3d pos = m_trans * m_zoom;

	// glLineWidth(2);
	m_objectLabels->Clear();
	if (m_system->GetUnexplored())
		m_infoLabel->SetText(Lang::UNEXPLORED_SYSTEM_NO_SYSTEM_VIEW);
	else {
		if (m_system->GetRootBody()) {
			// all systembodies draws here
			PutBody(m_system->GetRootBody().Get(), pos, trans);
		}
	}
	// glLineWidth(1);


	if (m_game->GetSpace()->GetStarSystem()->GetPath().IsSameSystem(m_game->GetSectorView()->GetSelected()))
	{
		// draw ships
		if (m_shipDrawing != OFF) {
			RefreshShips();
			DrawShips(m_time, pos);
		}
		// draw player and planner
		vector3d ppos(0.0);
		Orbit playerOrbit = Pi::player->ComputeOrbit();
		Body* PlayerBody = static_cast<Body*>(Pi::player);
		FrameId playerNonRotFrameId = Frame::GetFrame(PlayerBody->GetFrame())->GetNonRotFrame();
		Frame* playerNonRotFrame = Frame::GetFrame(playerNonRotFrameId);
		SystemBody* playerAround = playerNonRotFrame->GetSystemBody();
		CalculateShipPositionAtTime(static_cast<Ship*>(Pi::player), playerOrbit, m_time, ppos);
		AddNotProjected<Body>(Projectable::PLAYERSHIP, PlayerBody, ppos * m_zoom + pos);

		vector3d offset(0.0);
		CalculateFramePositionAtTime(playerNonRotFrameId, m_time, offset);
		offset = offset * m_zoom + pos;

		if (Pi::player->GetFlightState() == Ship::FlightState::FLYING)
		{
			PutOrbit<Body>(PlayerBody, &playerOrbit, offset, Color::RED, playerAround->GetRadius());
			const double plannerStartTime = m_planner->GetStartTime();
			if (!m_planner->GetPosition().ExactlyEqual(vector3d(0, 0, 0))) {
				Orbit plannedOrbit = Orbit::FromBodyState(m_planner->GetPosition(),
						m_planner->GetVel(),
						playerAround->GetMass());
				PutOrbit<Body>(PlayerBody, &plannedOrbit, offset, Color::STEELBLUE, playerAround->GetRadius());
				if (std::fabs(m_time - m_game->GetTime()) > 1. && (m_time - plannerStartTime) > 0.)
					AddNotProjected<Body>(Projectable::PLANNER, PlayerBody, offset + plannedOrbit.OrbitalPosAtTime(m_time - plannerStartTime) * static_cast<double>(m_zoom));
				else
					AddNotProjected<Body>(Projectable::PLANNER, PlayerBody, offset + m_planner->GetPosition() * static_cast<double>(m_zoom));
			}
		}
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
	// offset - translate vector to selected object, scaled to camera scale
	for (auto s = m_contacts.begin(); s != m_contacts.end(); s++) {
		vector3d pos(0.0);
		CalculateShipPositionAtTime((*s).first, (*s).second, t, pos);
		pos = pos * m_zoom + offset;
		//draw green orbit for selected ship
		const bool isSelected = m_selectedObject.type == Projectable::OBJECT && m_selectedObject.reftype == Projectable::BODY && m_selectedObject.ref.body == (*s).first;
		LabelShip((*s).first, pos);
		if (m_shipDrawing == ORBITS && (*s).first->GetFlightState() == Ship::FlightState::FLYING)
		{
			vector3d framepos(0.0);
			CalculateFramePositionAtTime(Frame::GetFrame((*s).first->GetFrame())->GetNonRotFrame(), m_time, framepos);
			PutOrbit<Body>(static_cast<Body*>((*s).first), &(*s).second, offset + framepos * m_zoom, isSelected ? Color::GREEN : Color::BLUE, 0);
		}
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
	pos *= m_zoom;

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
		offset *= m_zoom;
		m_lineVerts->Add(vector3f(pos - offset), Color::GRAY * 0.5);
		offset.y = 0.0;
		m_lineVerts->Add(vector3f(pos - offset), Color::GRAY * 0.5);
	}

	m_lines.SetData(m_lineVerts->GetNumVerts(), &m_lineVerts->position[0], &m_lineVerts->diffuse[0]);
	m_lines.Draw(Pi::renderer, m_lineState);
}

template <typename T>
void SystemView::AddNotProjected(Projectable::types type, T *ref, const vector3d &worldscaledpos)
{
	//project and add
	Gui::Screen::EnterOrtho();
	vector3d pos;
	if (Gui::Screen::Project(worldscaledpos, pos) && pos.z < 1)
		AddProjected<T>(type, ref, pos);
	Gui::Screen::LeaveOrtho();
}

template <typename T>
void SystemView::AddProjected(Projectable::types type, T *ref, vector3d &pos)
{
	float scale[2];
	Gui::Screen::GetCoords2Pixels(scale);
	Projectable p(type, ref);
	p.screenpos.x = pos.x / scale[0];
	p.screenpos.y = pos.y / scale[1];
	p.screenpos.z = pos.z;
	m_projected.push_back(p);
}

// SystemBody can't be inaccessible
void SystemView::BodyInaccessible(Body *b)
{
	if (m_selectedObject.type == Projectable::OBJECT && m_selectedObject.reftype == Projectable::BODY && m_selectedObject.ref.body == b) ResetViewpoint();
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
		m_zoomTo *= 1 / pow(ZOOM_OUT_SPEED * Pi::GetMoveSpeedShiftModifier(), Pi::GetFrameTime());
	else Output("Unknown visibility: %s\n", param.c_str());
}

Projectable* SystemView::GetSelectedObject()
{
	return &m_selectedObject;
}

void SystemView::SetSelectedObject(Projectable::types type, SystemBody *sb)
{
	m_selectedObject.type = type;
	m_selectedObject.reftype = Projectable::SYSTEMBODY;
	m_selectedObject.ref.sbody = sb;
	m_animateTransition = MAX_TRANSITION_FRAMES;
}

void SystemView::SetSelectedObject(Projectable::types type, Body *b)
{
	m_selectedObject.type = type;
	m_selectedObject.reftype = Projectable::BODY;
	m_selectedObject.ref.body = b;
	m_animateTransition = MAX_TRANSITION_FRAMES;
}
