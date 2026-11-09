#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <Windows.h>
#include "Notifier.h"
#include <fstream>
#include <mutex>
#include <memory>

class CGlobal
{
public:
	CGlobal();
	~CGlobal();
	static HMODULE g_hInstance;

	// Thread-safe singleton implementation
	static CGlobal* Instance();

	// Old implementation kept for reference
	static void LogOld(const char* _Format, ...);
	// New simplified logging implementation
	static void Log(const char* _Format, ...);

public:
	std::string m_gameExe;
	std::string m_gameId;
	std::string m_idcToken;
public:
	Notifier*		m_notifier;
	class CLogger*	m_logger;

private:
	// Thread-safe singleton members
	static std::unique_ptr<CGlobal> instance_;
	static std::once_flag initialized_;
	static std::mutex log_mutex_;  // Protect logging operations
};
