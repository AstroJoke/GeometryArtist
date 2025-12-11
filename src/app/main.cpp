#ifdef _WIN32
#include <tchar.h>
#include <Windows.h>
#include <DbgHelp.h>
#include <minidumpapiset.h>

#ifdef UNICODE
#define TSprintf wsprintf
#else
#define TSprintf sprintf
#endif	

#pragma comment(lib, "dbghelp.lib")
void CreateDumpFile(LPCWSTR strPath, EXCEPTION_POINTERS* pException)
{
	HANDLE hDumpFile = CreateFile(strPath, GENERIC_WRITE, 0, NULL,
								  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDumpFile != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
		dumpInfo.ExceptionPointers = pException;
		dumpInfo.ThreadId = GetCurrentThreadId();
		dumpInfo.ClientPointers = TRUE;

		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
						  hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
		CloseHandle(hDumpFile);
	}
}

LONG ApplicationCrashHandler(EXCEPTION_POINTERS* pException)
{
	TCHAR szDumpDir[MAX_PATH] = { 0 };
	TCHAR szDumpFile[MAX_PATH] = { 0 };
	TCHAR szMsg[MAX_PATH] = { 0 };
	SYSTEMTIME	stTime = { 0 };

	GetLocalTime(&stTime);
	::GetCurrentDirectory(MAX_PATH, szDumpDir);
	TSprintf(szDumpFile, _T("%s\\%04d%02d%02d_%02d%02d%02d.dmp"), szDumpDir,
		stTime.wYear, stTime.wMonth, stTime.wDay,
		stTime.wHour, stTime.wMinute, stTime.wSecond);
	CreateDumpFile(szDumpFile, pException);

	TSprintf(szMsg, _T("I'm so sorry, but the program crashed.\r\ndump file : %s"), szDumpFile);
	FatalAppExit(-1, szMsg);
	return EXCEPTION_EXECUTE_HANDLER;
}

void EnableAutoDump(bool bEnable)
{
	if (bEnable)
	{
		SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler);
	}
}
#endif

#include <memory>

#include <QApplication>
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include <QOffscreenSurface>

#include "geometry_artist_main.h"

bool checkOpenGLVersion(int major, int minor)
{
	QOpenGLContext context;
	QOffscreenSurface surface;
	surface.create();

	if (context.create() && context.makeCurrent(&surface))
	{
		QSurfaceFormat format = context.format();
		bool ok = (format.majorVersion() > major) ||
				  (format.majorVersion() == major && format.minorVersion() >= minor);
		context.doneCurrent();
		return ok;
	}
	return false;
}

int main(int argc, char* argv[])
{

#ifdef _WIN32
	EnableAutoDump(true);
#endif 

	QApplication app(argc, argv);

	app.setApplicationName("GeometryArtist");
	app.setApplicationVersion("0.1.0");

	QSurfaceFormat fmt;
	fmt.setRenderableType(QSurfaceFormat::OpenGL);

	// try Core Profile 4.3
	if (checkOpenGLVersion(4, 3)) {
		fmt.setVersion(4, 3);
		fmt.setProfile(QSurfaceFormat::CoreProfile);
	}
	// try Core Profile 3.3
	else if (checkOpenGLVersion(3, 3)) {
		fmt.setVersion(3, 3);
		fmt.setProfile(QSurfaceFormat::CoreProfile);
	}
	// lowest require
	else {
		fmt.setVersion(2, 1);
		fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
	}

	fmt.setRenderableType(QSurfaceFormat::OpenGL);

#if ENABLE_OPENGL_DEBUG
	fmt.setOption(QSurfaceFormat::DebugContext);
#endif

	constexpr int sampleCount = 4;  // 4x MSAA
	fmt.setSamples(sampleCount);
	QSurfaceFormat::setDefaultFormat(fmt);

	std::unique_ptr<geomart::GeometryArtistMain> ptrMainWindow = 
		std::make_unique<geomart::GeometryArtistMain>();
	ptrMainWindow->show();

	return app.exec();
}