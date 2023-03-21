#pragma once

static bool glewExperimental = false;
static int GLEW_OK = 1;
static int GLEW_VERSION = 1;

inline int glewInit() { return GLEW_OK; }
inline const char *glewGetErrorString(int) { return "MOCK!"; }
inline int glewIsSupported(const char *) { return GLEW_OK; }
inline const char *glewGetString(int) { return "fake_glew_string"; }
