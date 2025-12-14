/*
 * Copyright (c) 2020-2023 Alex Spataru <https://github.com/alex-spataru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <QThread>
#include <QSysInfo>
#include <QSettings>
#include <QQuickStyle>
#include <QApplication>
#include <QStyleFactory>
#include <QCommandLineParser>

#include "AppInfo.h"
#include "IO/Manager.h"
#include "IO/Drivers/UART.h"
#include "JSON/ProjectModel.h"
#include "Misc/ModuleManager.h"

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <cstring>
#endif

#ifdef Q_OS_LINUX
#  include <QDir>
#  include <QFile>
#  include <QFileInfo>
#  include <QStandardPaths>
#endif

//------------------------------------------------------------------------------
// Declare utility functions
//------------------------------------------------------------------------------

static void cliShowVersion();
static void cliResetSettings();

#ifdef Q_OS_LINUX
static void setupAppImageIcon(const QString &appExecutableName,
                              const QString &iconResourcePath);
#endif

#ifdef Q_OS_WINDOWS
static void attachToConsole();
static char **adjustArgumentsForFreeType(int &argc, char **argv);
#endif

//------------------------------------------------------------------------------
// Entry-point function
//------------------------------------------------------------------------------

/**
 * @brief Entry-point function of the application
 *
 * @param argc argument count
 * @param argv argument data
 *
 * @return qApp exit code
 */
int main(int argc, char **argv)
{
  // Set application info
  QApplication::setApplicationName(APP_EXECUTABLE);
  QApplication::setOrganizationName(APP_DEVELOPER);
  QApplication::setApplicationVersion(APP_VERSION);
  QApplication::setApplicationDisplayName(APP_NAME);
  QApplication::setOrganizationDomain(APP_SUPPORT_URL);

  // Windows specific initialization code
#ifdef Q_OS_WIN
  attachToConsole();
  argv = adjustArgumentsForFreeType(argc, argv);
#endif

  // Linux specific initialization code
#ifdef Q_OS_LINUX
  setupAppImageIcon(APP_EXECUTABLE, QStringLiteral(":/rcc/images/icon@2x.png"));
#endif

  // Avoid 200% scaling on 150% scaling...
  auto policy = Qt::HighDpiScaleFactorRoundingPolicy::PassThrough;
  QApplication::setHighDpiScaleFactorRoundingPolicy(policy);

  // Initialize application
  QApplication app(argc, argv);

  // Set thread priority
  QThread::currentThread()->setPriority(QThread::HighestPriority);

  // Set application style
  app.setStyle(QStyleFactory::create("Fusion"));
  QQuickStyle::setStyle("Fusion");

  // Setup command line parser
  QCommandLineParser parser;
  parser.setApplicationDescription("Multi-platform dashboard for embedded systems");
  parser.addHelpOption();

  // Add CLI options
  QCommandLineOption versionOption(
      QStringList() << "v"
                    << "version",
      "Show application version");
  QCommandLineOption resetOption(
      QStringList() << "r"
                    << "reset",
      "Reset application settings");
  QCommandLineOption deviceOption("device",
                                  "Serial device to connect (e.g., /dev/ttyUSB0)",
                                  "device");
  QCommandLineOption baudOption("baud", "Baud rate for serial connection (e.g., 115200)",
                                "baudrate");
  QCommandLineOption projectOption("project", "Project file to load", "file");

  parser.addOption(versionOption);
  parser.addOption(resetOption);
  parser.addOption(deviceOption);
  parser.addOption(baudOption);
  parser.addOption(projectOption);

  // Process arguments
  parser.process(app);

  // Handle version option
  if (parser.isSet(versionOption))
  {
    cliShowVersion();
    return EXIT_SUCCESS;
  }

  // Handle reset option
  if (parser.isSet(resetOption))
  {
    cliResetSettings();
    return EXIT_SUCCESS;
  }

  // Create module manager
  Misc::ModuleManager moduleManager;
  moduleManager.configureUpdater();

  // Initialize QML interface
  moduleManager.registerQmlTypes();
  moduleManager.initializeQmlInterface();
  if (moduleManager.engine().rootObjects().isEmpty())
  {
    qCritical() << "Critical QML error";
    return EXIT_FAILURE;
  }

  // Handle project file option
  if (parser.isSet(projectOption))
  {
    QString projectPath = parser.value(projectOption);
    JSON::ProjectModel::instance().openJsonFile(projectPath);
  }

  // Handle device and baud rate options
  if (parser.isSet(deviceOption) || parser.isSet(baudOption))
  {
    // Set bus type to UART
    IO::Manager::instance().setBusType(SerialStudio::BusType::UART);

    // Set device if provided
    if (parser.isSet(deviceOption))
    {
      QString device = parser.value(deviceOption);
      IO::Drivers::UART::instance().registerDevice(device);
    }

    // Set baud rate if provided
    if (parser.isSet(baudOption))
    {
      bool ok;
      qint32 baudRate = parser.value(baudOption).toInt(&ok);
      if (ok)
        IO::Drivers::UART::instance().setBaudRate(baudRate);
      else
        qWarning() << "Invalid baud rate:" << parser.value(baudOption);
    }

    // Connect to device
    IO::Manager::instance().connectDevice();
  }

  // Enter application event loop
  const auto status = app.exec();

  // Free dynamically-generated argv on Windows
#ifdef Q_OS_WIN
  for (int i = 0; i < argc; ++i)
    free(argv[i]);

  delete[] argv;
#endif

  // Exit application
  return status;
}

//------------------------------------------------------------------------------
// Implement utility functions
//------------------------------------------------------------------------------

/**
 * Prints the current application version to the console
 */
static void cliShowVersion()
{
  qDebug() << APP_NAME << "version" << APP_VERSION;
  qDebug() << "Written by Alex Spataru <https://github.com/alex-spataru>";
}

/**
 * Removes all application settings
 */
static void cliResetSettings()
{
  QSettings(APP_SUPPORT_URL, APP_NAME).clear();
  qDebug() << APP_NAME << "settings cleared!";
}

//------------------------------------------------------------------------------
// Linux-specific initialization code
//------------------------------------------------------------------------------

#ifdef Q_OS_LINUX
/**
 * @brief Ensures the application icon is set correctly for AppImage deployments
 *        on GNU/Linux.
 *
 * This function copies the application icon from the resource file to the
 * appropriate icon directory on GNU/Linux systems.
 *
 * If the icon file does not already exist in the local user's icons directory,
 * it is copied from the application’s resources to ensure proper display in the
 * desktop environment, even when running the application as an AppImage.
 *
 * The function creates any necessary directories if they do not exist, and
 * performs file checks to prevent redundant copying.
 *
 * @param appExecutableName The name of the application executable, used to name
 *                          the icon file.
 * @param iconResourcePath The path to the icon in the application's resources
 *                         (e.g., `:/rcc/images/icon@2x.png`).
 */
static void setupAppImageIcon(const QString &appExecutableName,
                              const QString &iconResourcePath)
{
  // clang-format off
  const QString pixmapPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/icons/hicolor/256x256/apps/";
  const QString pixmapFile = pixmapPath + appExecutableName + ".png";
  // clang-format on

  // Check if the icon file already exists to avoid redundant copying
  if (!QFileInfo::exists(pixmapFile))
  {
    // Ensure the directory exists; create it if it doesn't
    QDir dir;
    if (!dir.exists(pixmapPath) && !dir.mkpath(pixmapPath))
      return;

    // Copy the icon from resources to the destination
    QFile resourceFile(iconResourcePath);
    if (resourceFile.open(QIODevice::ReadOnly))
    {
      QFile localFile(pixmapFile);
      if (localFile.open(QIODevice::WriteOnly))
      {
        localFile.write(resourceFile.readAll());
        localFile.close();
      }

      resourceFile.close();
    }
  }
}
#endif

//------------------------------------------------------------------------------
// Windows-specific initialization code
//------------------------------------------------------------------------------

#ifdef Q_OS_WIN
/**
 * @brief Attaches the application to the parent console and redirects output
 *        streams on Windows.
 *
 * This function attaches the application to the parent process's console if it
 * was launched from the command prompt. It redirects the `stdout` and `stderr`
 * streams to the console to enable proper output display. Additionally, it
 * prints a newline to avoid overlapping text with any previous user commands
 * in the console.
 */
static void attachToConsole()
{
  if (AttachConsole(ATTACH_PARENT_PROCESS))
  {
    (void)freopen_s(nullptr, "CONOUT$", "w", stdout);
    (void)freopen_s(nullptr, "CONOUT$", "w", stderr);
    printf("\n");
  }
}

/**
 * @brief Adjusts command-line arguments to enable FreeType font rendering on
 *        Windows.
 *
 * This function forces the application to use FreeType font rendering instead
 * of DirectWrite or GDI. This approach fixes pixelated fonts being shown in
 * the user interface in screens whose scale factor is 100%.
 *
 * @param argc Reference to the argument count from `main()`.
 * @param argv Array of command-line arguments from `main()`.
 */
static char **adjustArgumentsForFreeType(int &argc, char **argv)
{
  // Define the additional FreeType arguments for Windows
  const char *platformArgument = "-platform";
  const char *platformOption = "windows:fontengine=freetype";

  // Dynamically allocate a new array of char* pointers
  char **newArgv = new char *[argc + 2];

  // Copy original argv into newArgv
  for (int i = 0; i < argc; ++i)
    newArgv[i] = _strdup(argv[i]);

  // Add new arguments to the end
  newArgv[argc] = const_cast<char *>(platformArgument);
  newArgv[argc + 1] = const_cast<char *>(platformOption);

  // Update argc and return the new array
  argc += 2;
  return newArgv;
}
#endif
