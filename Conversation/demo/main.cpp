#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include "Conversation.h"
//#include "variant.h"
//#include "dialogue.h"

/*
std::ostream& operator<<(std::ostream& out, const variant& v)
{
	switch (v.get_type())
	{
	case variant::VT_INT: out << (int)(v); break;
	case variant::VT_FLOAT: out << (float)(v); break;
	case variant::VT_BOOL: out << ((bool)(v) ? "true" : "false"); break;
	}
	return out;
}
*/

const char* ACT_SET = ":=";
const char* ACT_INC = "+=";
const char* ACT_DEC = "-=";
const char* ACT_NOT = "!=";


Conversation* currentConversation = nullptr;
std::vector<Conversation*> allConversations;


#include "pugixml\pugixml.hpp"
template <typename T>

inline void AddNodeAttrib(pugi::xml_node& node, const char* Name, T m_value)
{
	node.append_attribute(Name).set_value(m_value);
}

void ShowProperties();
void DoConversation(Conversation* conv)
{
	Save* save = CreateSave();
	SetConversationSave(GetContext(), save);

	currentConversation = conv;

	Question* currentQuestion = StartConversation(conv);

	while (currentQuestion != nullptr)
	{
		std::cout << "当前对话人：" << std::endl << std::endl;
		ShowProperties();

		std::cout << GetQuestionText(currentQuestion)
			<< std::endl << std::endl;


		int answer_count = GetAnswerCount(currentQuestion);
		if (answer_count == 0)
		{
			break;
		}

		int i = 1;
		for (; i <= answer_count; i++)
		{
			Answer* answer = GetAnswerByIndex(currentQuestion, i - 1);
			if (IsAnswerShown(answer))
			{
				std::cout << i << "-"
					<< GetAnswerText(answer)
					<< std::endl;
			}
		}

		std::cout << std::endl << "选择回答： （退出输入别的标号）";
		std::cin >> i;
		if (i <= 0 || i > answer_count)
		{
			break;
		}

		i--;
		Answer* answer = GetAnswerByIndex(currentQuestion, i);
		if (IsAnswerShown(answer))
		{
			currentQuestion = GetNextQuestion(conv, answer);
			system("cls");
		}
		else
		{
			break;
		}
	}
	ReleaseSave(save);
}

void TraversalDialogueFolder();
void CreateExampleXML(const char*);

int main()
{
	//构建example
	CreateExampleXML("example.xml");

	//载入对话数据
	TraversalDialogueFolder();

	InitialContext();

	//show dialogue
	int conversationCount = static_cast<int>(allConversations.size());
	if (conversationCount > 0)
	{
		while (true)
		{
			system("cls");
			int i = 1;
			for (auto& conv : allConversations)
			{
				if (IsConversationValid(conv))
				{
					std::cout << i << ". " << GetConversationID(conv) << std::endl;
				}
				i++;
			}

			std::cout << std::endl << "选择对话人： （退出输入别的标号）";
			std::cin >> i;
			if (i <= 0 || i > conversationCount)
			{
				break;
			}

			system("cls");
			if (!IsConversationValid(allConversations[i - 1]))
			{
				std::cout << "你的选择有雾(XD.....，他不在这个场景里" << std::endl;
			}
			else
			{
				DoConversation(allConversations[i - 1]);
			}
		}

		for (int i = 0; i < conversationCount; i++)
		{
			ReleaseConversation(allConversations[i]);
		}
	}
	return 0;
}

void ShowProperties()
{
	/*
	typedef Prop& (*GetPropFunc)();
	const int block_count = 4;
	struct
	{
		std::string Name;
		GetPropFunc func;
	} blocks[block_count] =
	{
		{ "全局", GetPropGlobal },
		{ "对话", GetPropDialog},
		{ "角色", GetPropSelf },
		{ "NPC",  GetPropNPC },
	};

	for (int i = 0; i < block_count; i++)
	{
		Prop& prop = blocks[i].func();
		int prop_count = static_cast<int>(prop.size());
		if (prop_count)
		{
			std::cout << blocks[i].Name.c_str() << "属性：" << std::endl;
			for (auto& p : prop)
			{
				std::cout << "\t" << p.first.c_str() << ":"
					<< p.second << std::endl;
			}

			std::cout << std::endl;
		}
	}
	*/
}

/*
Prop& GetPropNPC()
{
	return GetDialoguePeopleProp(g_current_dialog);
}

Prop& GetPropDialog()
{
	return GetDialogueProp(g_current_dialog);
}
*/

void TraversalDialogueFolder()
{
	WIN32_FIND_DATAA find_data;
	HANDLE find_ret;

	std::vector<std::string> xml_files;
	find_ret = ::FindFirstFileA("*.xml", &find_data);
	do
	{
		if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			continue;
		}
		std::string file_name = "";
		file_name += find_data.cFileName;
		xml_files.push_back(file_name);
	} while (TRUE == FindNextFileA(find_ret, &find_data));
	::FindClose(find_ret);

	for (auto& Name : xml_files)
	{
		Conversation* dialog = CreateConversationFromXML(Name.c_str());
		if (dialog)
		{
			allConversations.push_back(dialog);
		}
		else
		{
			std::cout << "error file:" << Name.c_str() << std::endl;
		}
	}
}

void CreateExampleXML(const char* file_name)
{
	pugi::xml_document example_file;

	//create header
	{
		auto decl = example_file.append_child(pugi::node_declaration);
		AddNodeAttrib(decl, "version", "1.0");
		AddNodeAttrib(decl, "encoding", "utf-8");
	}

	//构造子节点
	auto node_dialog = example_file.append_child("Conversation");
	auto node_entrances = node_dialog.append_child("Entrances");
	auto node_questions = node_dialog.append_child("Questions");

	//根节点属性
	AddNodeAttrib(node_dialog, "ID", "0");
	AddNodeAttrib(node_dialog, "ValidCondition", "player.is_dead == false");

	//构造入口列表
	{
		{
			auto node_goto = node_entrances.append_child("Branch");
			AddNodeAttrib(node_goto, "If", "conversation.count > 1");
			AddNodeAttrib(node_goto, "QuestionName", "another_default");
		}

		{
			auto node_goto = node_entrances.append_child("Branch");
			AddNodeAttrib(node_goto, "QuestionName", "default");
		}
	}

	//构造问题列表
	{
		//default question
		{
			auto node_question = node_questions.append_child("Question");
			AddNodeAttrib(node_question, "Name", "default");
			AddNodeAttrib(node_question, "Text", "你好，陌生人……");

			{
				auto node_answer = node_question.append_child("Answer");
				AddNodeAttrib(node_answer, "ID", "1");
				AddNodeAttrib(node_answer, "Text", "你是谁？");
				{
					auto node_goto = node_answer.append_child("Branch");
					AddNodeAttrib(node_goto, "QuestionName", "who_are_u");
				}
			}

			{
				auto node_answer = node_question.append_child("Answer");
				AddNodeAttrib(node_answer, "ID", "2");
				AddNodeAttrib(node_answer, "Text", "这儿是哪里？");
				{
					auto node_goto = node_answer.append_child("Branch");
					AddNodeAttrib(node_goto, "QuestionName", "where_am_i");
				}
			}
		}

		//question another_default
		{
			auto node_quesion = node_questions.append_child("Question");
			AddNodeAttrib(node_quesion, "Name", "another_default");
			AddNodeAttrib(node_quesion, "Text", "你还有什么想要了解的么？");

			{
				auto node_answer = node_quesion.append_child("Answer");
				AddNodeAttrib(node_answer, "ID", "1");
				AddNodeAttrib(node_answer, "Text", "你是谁？");
				AddNodeAttrib(node_answer, "ShowCondition", "conversation.last_id != 1 && conversation.count < 7");

				{
					auto node_actions = node_answer.append_child("Action");
					AddNodeAttrib(node_actions, "Do", "npc.love+=1");
				}

				{
					auto node_goto = node_answer.append_child("Branch");
					AddNodeAttrib(node_goto, "QuestionName", "who_are_u");
				}
			}

			{
				auto node_answer = node_quesion.append_child("Answer");
				AddNodeAttrib(node_answer, "ID", "2");
				AddNodeAttrib(node_answer, "Text", "这儿是哪里？");
				AddNodeAttrib(node_answer, "ShowCondition", "conversation.last_id != 2 || conversation.count >= 7");

				{
					auto node_actions = node_answer.append_child("Action");
					AddNodeAttrib(node_actions, "Do", "player.is_dead != player.is_dead");
				}

				{
					auto node_actions = node_answer.append_child("Action");
					AddNodeAttrib(node_actions, "Do", "player.hp := 100");
				}

				{
					auto node_actions = node_answer.append_child("Action");
					AddNodeAttrib(node_actions, "Do", "global.sunshine-=2.2");
					AddNodeAttrib(node_actions, "If", "global.sunshine > -10.0");
				}

				{
					auto node_goto = node_answer.append_child("Branch");
					AddNodeAttrib(node_goto, "QuestionName", "where_am_i");
				}
			}
		}

		//question answer_who_are_u
		{
			auto node_quesion = node_questions.append_child("Question");
			AddNodeAttrib(node_quesion, "Name", "who_are_u");
			AddNodeAttrib(node_quesion, "Text", "你猜！嘿嘿嘿...");

			{
				auto node_answer = node_quesion.append_child("Answer");
				AddNodeAttrib(node_answer, "ID", "1");
				AddNodeAttrib(node_answer, "Text", "好吧……");
			}
		}

		//question answer_where_am_i
		{
			auto node_quesion = node_questions.append_child("Question");
			AddNodeAttrib(node_quesion, "Name", "where_am_i");
			AddNodeAttrib(node_quesion, "Text", "oh，问得好，这得你自己去发现");

			{
				auto node_answer = node_quesion.append_child("Answer");
				AddNodeAttrib(node_answer, "ID", "2");
				AddNodeAttrib(node_answer, "Text", "好吧……");
			}
		}
	}

	example_file.save_file(file_name);
}