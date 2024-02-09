#include "JScr.h"
#include "Frontend/Parser.h"
using namespace JScr::Frontend;

namespace JScr
{
	Script::Result::Result(Script* script, const std::vector<SyntaxException>& errors) : errors(errors), script(script) {
        // Check that if script is null, errors must have more than zero items
        if (script == nullptr && (errors.size() == 0))
        {
            throw new std::invalid_argument("If script is null, errors must have more than zero items");
        }

        // Check that if script is not null, errors must be empty
        if (script != nullptr && errors.size() > 0)
        {
            throw new std::invalid_argument("If script is not null, errors must be empty");
        }
	}

    Script::Result Script::FromFile(const std::string& filedir, const std::vector<ExternalResource>& externals = {})
    {
        Script script{};
        std::vector<SyntaxException> errors = {};

        script.m_filedir = filedir;
        script.m_resources = externals;

        BuildStandardLibraryResources(script);

        try
        {
            Parser parser{};
            script.m_program.emplace(parser.ProduceAST(script.m_filedir));
        }
        catch (SyntaxException e)
        {
            errors.push_back(e);
        }

        return Script::Result(errors.size() < 1 ? &script : NULL, errors);
    }

    void Script::Execute(const std::function<void(int)>& endCallback, bool anotherThread = true)
    {
        if (!m_program.has_value()) throw "Cannot execute script while 'm_program' is null! The script needs to be initialised first.";
        else if (m_isRunning) throw "Cannot execute script while it already is running.";

        m_isRunning = true;

        //if (anotherThread)
        //{
        //    std::future<void> result = std::async(std::launch::async, []() { Interpreter.EvaluateProgram(m_program, m_resources); });
        //    result.wait();
        //} else
        //    Interpreter.EvaluateProgram(m_program, resources);

        m_isRunning = false;
    }
}