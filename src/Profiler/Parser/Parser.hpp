/**
 * @file Parser.hpp
 * @author Alexey Tsurikov
 *
 */

#ifndef _PARSER_H_
#define _PARSER_H_

void ParseFiles(const char *dir_name);
void UpdateFile(const char *file_name);
void ParseLine(std::string* _file_line, std::ofstream* fp_out);
void SearchText(std::string* __file_line, std::string __mytext, int* __numtext);
void SearchBrace(std::string* __pfile_line);
void SearchEqualSign(std::string* __file_line, int* __numtext);
void SaveFunctionLog(unsigned int func_number, std::ofstream* _log_out, std::ofstream* _dot_out);

#endif
