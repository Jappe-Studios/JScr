#include "JScr.h"
#include <future>

namespace JScr
{
	Script::Result::Result(Script* script, const std::list<SyntaxException>& errors) {
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

        this->script = script;
        this->errors = errors;
	}

    Script::Result Script::FromFile(const std::string& filedir, const std::list<ExternalResource>& externals = {})
    {
        Script script{};
        std::list<SyntaxException> errors = {};

        script.m_filedir = filedir;
        script.m_resources = externals;

        BuildStandardLibraryResources(script);

        try
        {
            Parser parser{};

            std::ifstream file(filedir);
            std::vector<char> buffer;
            if (file)
            {
                file.seekg(0, std::ios::end);
                std::streampos length = file.tellg();
                file.seekg(0, std::ios::beg);

                buffer.resize(length);
                file.read(&buffer[0], length);
            }

            script.m_program = parser.ProduceAST(script.m_filedir, buffer);
        }
        catch (SyntaxException e)
        {
            errors.push_back(e);
        }

        return Script::Result(errors.size() < 1 ? &script : NULL, errors);
    }

    void Script::Execute(const std::function<void(int)>& endCallback, const bool& anotherThread = true)
    {
        if (m_program == NULL) throw "Cannot execute script while 'm_program' is null! The script needs to be initialised first.";
        else if (IsRunning) throw "Cannot execute script while it already is running.";

        m_isRunning = true;

        if (anotherThread)
        {
            std::future<void> result = std::async(std::launch::async, []() { Interpreter.EvaluateProgram(m_program, m_resources); });
            result.wait();
        } else
            Interpreter.EvaluateProgram(m_program, resources);

        m_isRunning = false;
    }
}