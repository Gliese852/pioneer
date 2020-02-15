// Copyright © 2008-2020 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _SYSTEMVIEW_H
#define _SYSTEMVIEW_H

#include "Color.h"
#include "UIView.h"
#include "graphics/Drawables.h"
#include "matrix4x4.h"
#include "vector3.h"
#include "LuaTable.h"
#include "LuaPiGui.h"

class StarSystem;
class SystemBody;
class Orbit;
class Ship;
class Game;
class Body;

enum BurnDirection {
	PROGRADE,
	NORMAL,
	RADIAL,
};

enum ShipDrawing {
	BOXES,
	ORBITS,
	OFF
};

enum class GridDrawing {
	GRID,
	GRID_AND_LEGS,
	OFF
};

enum ShowLagrange {
	LAG_ICON,
	LAG_ICONTEXT,
	LAG_OFF
};

class TransferPlanner {
public:
	TransferPlanner();
	vector3d GetVel() const;
	vector3d GetOffsetVel() const;
	vector3d GetPosition() const;
	double GetStartTime() const;
	void SetPosition(const vector3d &position);
	void IncreaseFactor(), ResetFactor(), DecreaseFactor();
	void AddStartTime(double timeStep);
	void ResetStartTime();
	double GetFactor() const { return m_factor; }
	void AddDv(BurnDirection d, double dv);
	double GetDv(BurnDirection d);
	void ResetDv(BurnDirection d);
	void ResetDv();
	std::string printDeltaTime();
	std::string printDv(BurnDirection d);
	std::string printFactor();

private:
	double m_dvPrograde;
	double m_dvNormal;
	double m_dvRadial;
	double m_factor; // dv multiplier
	const double m_factorFactor = 5.0; // m_factor multiplier
	vector3d m_position;
	vector3d m_velocity;
	double m_startTime;
};

typedef std::vector<std::pair<std::pair<const Body*,const SystemBody*>, vector3d>> BodyPositionVector;

class SystemView: public UIView {
public:
	SystemView(Game *game);
	virtual ~SystemView();
	virtual void Update();
	virtual void Draw3D();
	const Body *GetSelectedObject() const { return m_selectedObject; }
	double GetOrbitPlannerStartTime() const { return m_planner->GetStartTime(); }
	double GetOrbitPlannerTime() const { return m_time; }
	void OnClickAccel(float step);
	void OnClickRealt();
	TSS_vector GetProjectedBodies() const { return m_projectedBodies; }
	bool SetSelectedObject(Body* b);
	void BodyInaccessible(Body *b);
	void SetVisibility(std::string param);
private:
	TSS_vector m_projectedBodies;
	static const double PICK_OBJECT_RECT_SIZE;
	static const Uint16 N_VERTICES_MAX;
	const double m_camera_fov = 50.f;
	void PutOrbit(const Orbit *orb, const vector3d &offset, const Color &color, const double planetRadius = 0.0, const bool showLagrange = false);
	void PutBody(const SystemBody *b, const vector3d &offset, const matrix4x4f &trans);
	void PutLabel(const SystemBody *b, const vector3d &offset);
	void PutSelectionBox(const SystemBody *b, const vector3d &rootPos, const Color &col);
	void PutSelectionBox(const vector3d &worldPos, const Color &col);
	void GetTransformTo(const SystemBody *b, vector3d &pos);
	void GetTransformTo(const Body *b, vector3d &pos);
	void OnClickLagrange();
	void ResetViewpoint();
	void MouseWheel(bool up);
	void RefreshShips(void);
	void DrawShips(const double t, const vector3d &offset);
	void PrepareGrid();
	void DrawGrid();
	void LabelShip(Ship *s, const vector3d &offset);
	void AddProjectedBody(Body *b, vector3d pos, vector3d worldpos);

	Game *m_game;
	RefCountedPtr<StarSystem> m_system;
	const Body *m_selectedObject;
	std::vector<SystemBody *> m_displayed_sbody;
	bool m_unexplored;
	ShowLagrange m_showL4L5;
	TransferPlanner *m_planner;
	std::list<std::pair<Ship *, Orbit>> m_contacts;
	ShipDrawing m_shipDrawing;
	GridDrawing m_gridDrawing;
	int m_grid_lines;
	float m_rot_x, m_rot_y;
	float m_rot_x_to, m_rot_y_to;
	float m_zoom, m_zoomTo;
	double m_time;
	bool m_realtime;
	double m_timeStep;
	Gui::Label *m_infoLabel;
	Gui::Label *m_infoText;
	Gui::LabelSet *m_objectLabels;
	sigc::connection m_onMouseWheelCon;

	std::unique_ptr<Graphics::Drawables::Disk> m_bodyIcon;
	std::unique_ptr<Gui::TexturedQuad> m_l4Icon;
	std::unique_ptr<Gui::TexturedQuad> m_l5Icon;
	std::unique_ptr<Gui::TexturedQuad> m_periapsisIcon;
	std::unique_ptr<Gui::TexturedQuad> m_apoapsisIcon;
	Graphics::RenderState *m_lineState;
	Graphics::Drawables::Lines m_orbits;
	Graphics::Drawables::Lines m_selectBox;

	std::unique_ptr<vector3f[]> m_orbitVts;
	std::unique_ptr<Color[]> m_orbitColors;

	std::unique_ptr<Graphics::VertexArray> m_lineVerts;
	Graphics::Drawables::Lines m_lines;

};

#endif /* _SYSTEMVIEW_H */
