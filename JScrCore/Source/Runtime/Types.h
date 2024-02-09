#pragma once
#include <unordered_map>
#include <string>
#include <memory>
using std::vector;
using std::string;

namespace JScr::Runtime
{
	class Types
	{
	public:
		class Type
		{
		public:
			const unsigned short& Uid() const { return m_uid; }
			const vector<Type>& LambdaTypes() const { return m_lambdaTypes; }
			const std::shared_ptr<Type> Child() const { return m_child; }
			const string& Data() const { return m_data; }

			const bool isLambda;

			Types::Type CopyWithLambdaTypes(const vector<Type>& lambdaTypes) { return Type(this->m_uid, lambdaTypes, this->m_child, this->m_data); };

		public:
			static Type Array(Type of, vector<Type> lambdaTypes = {})
			{
				return Type(0, std::move(lambdaTypes), std::make_shared<Type>(std::move(of)));
			}

			static Type Dynamic(vector<Type> lambdaTypes = {})
			{
				return Type(1, std::move(lambdaTypes));
			}

			static Type Object(string name, vector<Type> lambdaTypes = {})
			{
				return Type(2, std::move(lambdaTypes), nullptr, std::move(name));
			}

			static Type Void(vector<Type> lambdaTypes = {})
			{
				return Type(3, std::move(lambdaTypes));
			}

			static Type Bool(vector<Type> lambdaTypes = {})
			{
				return Type(4, std::move(lambdaTypes));
			}

			static Type Int(vector<Type> lambdaTypes = {})
			{
				return Type(5, std::move(lambdaTypes));
			}

			static Type Float(vector<Type> lambdaTypes = {})
			{
				return Type(6, std::move(lambdaTypes));
			}

			static Type Double(vector<Type> lambdaTypes = {})
			{
				return Type(7, std::move(lambdaTypes));
			}

			static Type String(vector<Type> lambdaTypes = {})
			{
				return Type(8, std::move(lambdaTypes));
			}

			static Type Char(vector<Type> lambdaTypes = {})
			{
				return Type(9, std::move(lambdaTypes));
			}

		private:
			Type(unsigned short uid, vector<Type> lambdaTypes, std::shared_ptr<Type> child = nullptr, string data = "") 
				  : m_uid(uid), m_lambdaTypes(std::move(lambdaTypes)), m_child(std::move(child)), m_data(std::move(data)), isLambda(!lambdaTypes.empty())
			{}
		
			unsigned short m_uid;
			vector<Type> m_lambdaTypes;
			std::shared_ptr<Type> m_child;
			string m_data;
		};

		static const std::unordered_map<Types::Type, std::string> types;

		static Type FromString(const string& input);
	};
}