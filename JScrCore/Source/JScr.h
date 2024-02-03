#pragma once
#include <list>
#include <fstream>
#include "Frontend/SyntaxException.h"

namespace JScr
{
	class Script
	{
	public:
		struct Result
		{
		public:
			Script* script;
			std::list<SyntaxException>& errors;
			const bool IsSuccess() const { return script != nullptr; };

			Result(Script* script, const std::list<SyntaxException>& errors);
		};

		const std::string fileExtension = ".jscr";

	public:
		const std::string& GetFileDir() { return m_filedir; };
		const bool& IsRunning() const { return m_isRunning; };

		static Result FromFile(const std::string& filedir, const std::list<ExternalResource>& externals = {});

		void Execute(const std::function<void(int)>& endCallback, const bool& anotherThread = true);

	private:
		static void BuildStandardLibraryResources(Script& script);

	private:
		std::string m_filedir;
		bool m_isRunning;

		std::list<ExternalResource> m_resources;

		Script();
	};

	__interface ExternalResource {};

	class ExternalResourceFile : public ExternalResource
	{
	public:
		ExternalResourceFile(const std::list<std::string>& location, const std::string& internalFile);
	};
}