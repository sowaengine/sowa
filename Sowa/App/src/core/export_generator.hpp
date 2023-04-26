#ifndef _E_EXPORTGENERATOR_HPP__
#define _E_EXPORTGENERATOR_HPP__

#include "core/engine_context.hpp"
#include "stlpch.hpp"

namespace sowa {
enum class ExportPlatform {
	NONE = 0,
	LINUX,
};

class ExportGenerator {
  public:
	ExportGenerator(EngineContext &ctx);
	~ExportGenerator();

	bool BeginExport(ExportPlatform _platform);
	bool EndExport();

	/**
	 * @brief Adds given file to build process
	 *
	 * @param filepath filepath of file in project structure (res://file.escn)
	 * @param useGamePack whether file should be located in game pack or export project filesystem (set to false for files that user should be able to edit easily)
	 */
	bool AddFile(const std::filesystem::path &filepath, bool useGamePack = true);

	/**
	 * @brief
	 *
	 * @param srcPath filepath of file in project structure (res://file.escn)
	 * @param dstPath relative filepath of destination (file.escn only)
	 * @param useGamePack whether file should be located in game pack or export project filesystem (set to false for files that user should be able to edit easily)
	 */
	bool AddFileTo(const std::filesystem::path &srcPath, const std::filesystem::path &dstPath, bool useGamePack = true);

  private:
	EngineContext &_Ctx;

	ExportPlatform _platform{ExportPlatform::NONE};
	std::filesystem::path _buildDir{""};

	// Returns string representation of export target. like linux, windows etc.
	std::string GetPlatformString();

	// Resetted after every BeginExport, set to true if any error occurss on build stage
	bool _canExport = false;
	int _warningCount = 0;
};
} // namespace sowa

#endif // _E_EXPORTGENERATOR_HPP__