#include "import.h"
#include "manager.h"
#include "pyproxyobject.h"
#include "dotop.h"
#include "port.h"
#include "Hosting.h"
#include "moduleobject.h"
#include "remote_object.h"

namespace X
{
	extern XLoad* g_pXload;
	extern bool LoadPythonEngine();

}

X::AST::Scope* X::AST::ScopeProxy::GetParentScope()
{
	return nullptr;
}
bool X::AST::Import::CalcCallables(XlangRuntime* rt, XObj* pContext,
	std::vector<Scope*>& callables)
{
	ScopeProxy* pProxy = new ScopeProxy();
	callables.push_back(pProxy);
	return true;
}
bool X::AST::Import::FindAndLoadExtensions(XlangRuntime* rt,
	std::string& curModulePath,
	std::string& loadingModuleName)
{
	std::string loadDllName;
	bool bHaveDll = false;
	//search xlang.app folder first
	std::vector<std::string> candiateFiles;
	bool bRet = file_search(g_pXload->GetConfig().appPath,
		LibPrefix+loadingModuleName + ShareLibExt, candiateFiles);
	if (bRet && candiateFiles.size() > 0)
	{
		loadDllName = candiateFiles[0];
		bHaveDll = true;
	}
	//search xlang.engine folder first
	if (!bHaveDll)
	{
		bRet = file_search(g_pXload->GetConfig().xlangEnginePath,
			LibPrefix+loadingModuleName + ShareLibExt, candiateFiles);
		if (bRet && candiateFiles.size() > 0)
		{
			loadDllName = candiateFiles[0];
			bHaveDll = true;
		}
	}
	//check dll search path
	if (!bHaveDll && g_pXload->GetConfig().dllSearchPath)
	{
		std::string dllSearchPath(g_pXload->GetConfig().dllSearchPath);
		std::vector<std::string> paths = split(dllSearchPath, '\n');
		for (auto& p : paths) 
		{
			bRet = file_search(p,LibPrefix + loadingModuleName + ShareLibExt, candiateFiles);
			if (bRet && candiateFiles.size() > 0)
			{
				loadDllName = candiateFiles[0];
				bHaveDll = true;
				break;
			}
		}
	}
	if (!bHaveDll && !curModulePath.empty())
	{
		bRet = file_search(curModulePath, LibPrefix+loadingModuleName + ShareLibExt, candiateFiles);
		if (bRet && candiateFiles.size() > 0)
		{
			loadDllName = candiateFiles[0];
			bHaveDll = true;
		}
	}
	if (!bHaveDll)
	{
		std::vector<std::string> searchPaths;
		rt->M()->GetSearchPaths(searchPaths);
		for (auto& pa : searchPaths)
		{
			bRet = file_search(pa, LibPrefix+loadingModuleName + ShareLibExt, candiateFiles);
			if (bRet && candiateFiles.size() > 0)
			{
				loadDllName = candiateFiles[0];
				bHaveDll = true;
				break;
			}
		}
	}
	bool bOK = false;
	if (bHaveDll)
	{
		typedef void (*LOAD)(void* pHost,X::Value module);
		void* libHandle = LOADLIB(loadDllName.c_str());
		if (libHandle)
		{
			LOAD load = (LOAD)GetProc(libHandle, "Load");
			if (load)
			{
				ModuleObject* pModuleObj = new ModuleObject(rt->M());
				Value curModule = Value(pModuleObj);
				load((void*)g_pXHost, curModule);
			}
			bOK = true;
		}
	}
	return bOK;
}
bool X::AST::Import::FindAndLoadXModule(XlangRuntime* rt,
	std::string& curModulePath,
	std::string& loadingModuleName,
	Module** ppSubModule)
{
	std::string loadXModuleFileName;
	bool bHaveX = false;
	std::string prefixPath;
	if (!m_path.empty())
	{
		prefixPath = m_path;
		ReplaceAll(prefixPath, ".", Path_Sep_S);
	}
	
	std::vector<std::string> searchPaths;
	searchPaths.push_back(curModulePath);
	searchPaths.push_back(g_pXload->GetConfig().xlangEnginePath);

	auto search = [](std::string& loadingModuleName,
		std::vector<std::string>& searchPaths,
		std::string& loadXModuleName, std::string prefixPath)
	{
		for (auto& pa : searchPaths)
		{
			std::vector<std::string> candiateFiles;
			bool bRet = file_search(pa+ Path_Sep_S+ prefixPath, 
				loadingModuleName + ".x", candiateFiles);
			if (bRet && candiateFiles.size() > 0)
			{
				loadXModuleName = candiateFiles[0];
				return true;
			}
		}
		return false;
	};
	bHaveX = search(loadingModuleName, searchPaths, loadXModuleFileName, prefixPath);
	if (!bHaveX)
	{
		rt->M()->GetSearchPaths(searchPaths);
		bHaveX = search(loadingModuleName, searchPaths, loadXModuleFileName, prefixPath);
	}
	bool bOK = false;
	if (bHaveX)
	{
		std::string code;
		bOK = LoadStringFromFile(loadXModuleFileName, code);
		if (bOK)
		{
			unsigned long long moduleKey = 0;
			auto* pSubModule = Hosting::I().Load(loadXModuleFileName.c_str(),
				code.c_str(), (int)code.size(), moduleKey);
			if (pSubModule)
			{
				X::Value v0;
				std::vector<X::Value> passInParams;
				bOK = Hosting::I().Run(pSubModule, v0, passInParams);
			}
			*ppSubModule = pSubModule;
		}
	}
	return bOK;
}
bool X::AST::Import::Exec(XlangRuntime* rt,ExecAction& action, XObj* pContext, 
	Value& v, LValue* lValue)
{
	Scope* pMyScope = GetScope();
	if (m_from)
	{
		Value v0;
		if (m_from->Exec(rt,action, pContext, v0, nullptr))
		{
			m_path = v0.ToString();
		}
	}
	if (m_thru)
	{
		Value v0;
		if (m_thru->Exec(rt,action, pContext, v0, nullptr))
		{
			m_thruUrl = v0.ToString();
		}
	}
	for (auto& im : m_importInfos)
	{
		std::string varName;
		if (im.alias.empty())
		{
			varName = im.name;
		}
		else
		{
			varName = im.alias;
		}
		if (!m_thruUrl.empty())
		{
			XProxy* proxy = Manager::I().QueryProxy(m_thruUrl);
			if (proxy)
			{
				auto* remoteObj = new RemoteObject(proxy);
				remoteObj->SetObjName(im.name);
				v = Value(dynamic_cast<XObj*>(remoteObj));
				pMyScope->AddAndSet(rt, pContext, varName, v);
				//rt->M()->Add(rt, varName, nullptr, v);
				continue;
			}
		}
		else
		{
			if (Manager::I().QueryAndCreatePackage(rt, im.name, v))
			{
				if (pMyScope)
				{
					pMyScope->AddAndSet(rt, pContext, varName, v);
				}
				continue;
			}
		}

		//check if it is builtin
		if (m_path.empty())
		{
			bool bOK = Manager::I().QueryAndCreatePackage(rt,
				im.name,v);
			if (bOK)
			{
				if (pMyScope)
				{
					pMyScope->AddAndSet(rt, pContext, varName, v);
				}
				continue;
			}
		}
		else
		{
			std::string curPath;
			if (rt->M())
			{
				curPath = rt->M()->GetModulePath();
			}
			bool bLoaded = FindAndLoadExtensions(rt,curPath, m_path);
			bool bOK = Manager::I().QueryAndCreatePackage(rt,
				im.name, v);
			if (bOK)
			{
				if (pMyScope)
				{
					pMyScope->AddAndSet(rt, pContext, varName, v);
				}
				continue;
			}
		}
		//Check if it is X module
		std::string curPath = rt->M()->GetModulePath();
		Module* pSubModule = nullptr;
		bool bOK = FindAndLoadXModule(rt, curPath, im.name, &pSubModule);
		if (bOK && pSubModule != nullptr)
		{
			ModuleObject* pModuleObj = new ModuleObject(pSubModule);
			v = Value(pModuleObj);
			if (pMyScope)
			{
				pMyScope->AddAndSet(rt, pContext, varName, v);
			}
			//rt->M()->Add(rt, varName, nullptr, v);
			continue;
		}
		//then try Python Module
		//reserved import for python builtins, such as open, close etc
		//import python
		//if get this import, if enablePython is false, 
		//change to true and Load Python engine
		std::string moduleName = im.name;
		if (m_path.empty() && moduleName == "python")
		{
			if (!g_pXload->GetConfig().enablePython)
			{
				g_pXload->GetConfig().enablePython = true;
				LoadPythonEngine();
			}
			moduleName = "builtins";
			varName = "python";
		}
		if(g_pXload->GetConfig().enablePython)
		{
			auto* pProxyObj =
				new Data::PyProxyObject(rt, pContext,
					moduleName, m_path, curPath);
			v = Value(pProxyObj);
			if (pMyScope)
			{
				pMyScope->AddAndSet(rt, pContext, varName, v);
			}
			//rt->M()->Add(rt, varName, nullptr, v);
		}
	}
	return true;
}
std::string X::AST::Import::ConvertDotSeqToString(
	X::AST::Expression* expr)
{
	DotOp* dotOp = dynamic_cast<DotOp*>(expr);
	auto L0 = dotOp->GetL();
	auto R0 = dotOp->GetR();
	std::string leftName;
	std::string rightName;
	if (L0 && L0->m_type == ObType::Var)
	{
		leftName = (dynamic_cast<Var*>(L0))->GetNameString();
	}
	if (R0)
	{
		if (R0->m_type == ObType::Dot)
		{
			rightName = ConvertDotSeqToString(R0);
		}
		else if (R0->m_type == ObType::Var)
		{
			rightName = (dynamic_cast<Var*>(R0))->GetNameString();
		}
	}
	return leftName + "." + rightName;
};

void X::AST::Import::ScopeLayout()
{
	Operator::ScopeLayout();
	auto* pMyScope = GetScope();
	if (m_from)
	{
		m_from->SetParent(m_parent);
		m_from->ScopeLayout();
	}
	auto proc_AsOP = [&](Expression* expr)
	{
		AsOp* asOp = dynamic_cast<AsOp*>(expr);
		AST::Expression* L0 = asOp->GetL();
		auto R0 = asOp->GetR();
		std::string leftName;
		std::string rightName;
		if (L0)
		{
			if (L0->m_type == ObType::Var)
			{
				leftName = (dynamic_cast<Var*>(L0))->GetNameString();
			}
			else if (L0->m_type == ObType::Dot)
			{
				leftName = ConvertDotSeqToString(L0);
			}
		}
		if (R0 && R0->m_type == ObType::Var)
		{
			rightName = (dynamic_cast<Var*>(R0))->GetNameString();
		}
		ImportInfo importInfo;
		importInfo.name = leftName;
		importInfo.alias = rightName;
		m_importInfos.push_back(importInfo);
	};
	auto proc_Var = [&](Expression* expr)
	{
		auto name = (dynamic_cast<Var*>(expr))->GetNameString();
		ImportInfo importInfo;
		importInfo.name = name;
		m_importInfos.push_back(importInfo);
	};
	auto proc_List = [&](Expression* expr)
	{
		List* pList = dynamic_cast<List*>(expr);
		auto list = pList->GetList();
		for (auto expr0 : list)
		{
			switch (expr0->m_type)
			{
			case ObType::As:
				proc_AsOP(expr0);
				break;
			case ObType::Var:
				proc_Var(expr0);
				break;
			default:
				break;
			}
		}
	};
	if (m_imports)
	{//{var|AsOp}*
		switch (m_imports->m_type)
		{
		case ObType::List:
			proc_List(m_imports);
			break;
		case ObType::As:
			proc_AsOP(m_imports);
			break;
		case ObType::Var:
			proc_Var(m_imports);
			break;
		default:
			break;
		}
	}
	if (m_thru)
	{
		m_thru->SetParent(m_parent);
		m_thru->ScopeLayout();
	}
}
