#pragma once
#include <fstream>
#include <future>
#include <optional>
#include <vector>
#include "Frontend/SyntaxException.h"
#include "Frontend/Ast.h"
using namespace JScr::Frontend;

namespace JScr
{

	__interface ExternalResource {};

	class ExternalResourceFile : public ExternalResource
	{
	public:
		ExternalResourceFile(const std::vector<std::string>& location, const std::string& internalFile);
	};

	class Script
	{
	public:
		struct Result
		{
		public:
			Script* script;
			const std::vector<SyntaxException>& errors;
			const bool IsSuccess() const { return script != nullptr; };

			Result(Script* script, const std::vector<SyntaxException>& errors);
		};

		const std::string fileExtension = ".jscr";

	public:
		const std::string& GetFileDir() { return m_filedir; };
		const bool& IsRunning() const { return m_isRunning; };

		static Result FromFile(const std::string& filedir, const std::vector<ExternalResource>& externals);

		void Execute(const std::function<void(int)>& endCallback, bool anotherThread);

	private:
		static void BuildStandardLibraryResources(Script& script);

	private:
		std::string m_filedir;
		bool m_isRunning = false;
		std::optional<Program> m_program = std::nullopt;

		std::vector<ExternalResource> m_resources;

		Script() {}
	};
}