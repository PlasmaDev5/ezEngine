#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>

namespace ezTokenParseUtils
{
  void SkipWhitespace(const TokenStream& tokens, ezUInt32& inout_uiCurToken)
  {
    while (inout_uiCurToken < tokens.GetCount() && ((tokens[inout_uiCurToken]->m_iType == ezTokenType::Whitespace) || (tokens[inout_uiCurToken]->m_iType == ezTokenType::BlockComment) || (tokens[inout_uiCurToken]->m_iType == ezTokenType::LineComment)))
      ++inout_uiCurToken;
  }

  void SkipWhitespaceAndNewline(const TokenStream& tokens, ezUInt32& inout_uiCurToken)
  {
    while (inout_uiCurToken < tokens.GetCount() && ((tokens[inout_uiCurToken]->m_iType == ezTokenType::Whitespace) || (tokens[inout_uiCurToken]->m_iType == ezTokenType::BlockComment) || (tokens[inout_uiCurToken]->m_iType == ezTokenType::Newline) || (tokens[inout_uiCurToken]->m_iType == ezTokenType::LineComment)))
      ++inout_uiCurToken;
  }

  bool IsEndOfLine(const TokenStream& tokens, ezUInt32 uiCurToken, bool bIgnoreWhitespace)
  {
    if (bIgnoreWhitespace)
      SkipWhitespace(tokens, uiCurToken);

    if (uiCurToken >= tokens.GetCount())
      return true;

    return tokens[uiCurToken]->m_iType == ezTokenType::Newline || tokens[uiCurToken]->m_iType == ezTokenType::EndOfFile;
  }

  void CopyRelevantTokens(const TokenStream& source, ezUInt32 uiFirstSourceToken, TokenStream& inout_destination, bool bPreserveNewLines)
  {
    inout_destination.Reserve(inout_destination.GetCount() + source.GetCount() - uiFirstSourceToken);

    {
      // skip all whitespace at the start of the replacement string
      ezUInt32 i = uiFirstSourceToken;
      SkipWhitespace(source, i);

      // add all the relevant tokens to the definition
      for (; i < source.GetCount(); ++i)
      {
        if (source[i]->m_iType == ezTokenType::BlockComment || source[i]->m_iType == ezTokenType::LineComment || source[i]->m_iType == ezTokenType::EndOfFile || (!bPreserveNewLines && source[i]->m_iType == ezTokenType::Newline))
          continue;

        inout_destination.PushBack(source[i]);
      }
    }

    // remove whitespace at end of macro
    while (!inout_destination.IsEmpty() && inout_destination.PeekBack()->m_iType == ezTokenType::Whitespace)
      inout_destination.PopBack();
  }

  bool Accept(const TokenStream& tokens, ezUInt32& inout_uiCurToken, const char* szToken, ezUInt32* pAccepted)
  {
    SkipWhitespace(tokens, inout_uiCurToken);

    if (inout_uiCurToken >= tokens.GetCount())
      return false;

    if (tokens[inout_uiCurToken]->m_DataView == szToken)
    {
      if (pAccepted)
        *pAccepted = inout_uiCurToken;

      inout_uiCurToken++;
      return true;
    }

    return false;
  }

  bool Accept(const TokenStream& tokens, ezUInt32& inout_uiCurToken, ezTokenType::Enum type, ezUInt32* pAccepted)
  {
    SkipWhitespace(tokens, inout_uiCurToken);

    if (inout_uiCurToken >= tokens.GetCount())
      return false;

    if (tokens[inout_uiCurToken]->m_iType == type)
    {
      if (pAccepted)
        *pAccepted = inout_uiCurToken;

      inout_uiCurToken++;
      return true;
    }

    return false;
  }

  bool Accept(const TokenStream& tokens, ezUInt32& inout_uiCurToken, const char* szToken1, const char* szToken2, ezUInt32* pAccepted)
  {
    SkipWhitespace(tokens, inout_uiCurToken);

    if (inout_uiCurToken + 1 >= tokens.GetCount())
      return false;

    if (tokens[inout_uiCurToken]->m_DataView == szToken1 && tokens[inout_uiCurToken + 1]->m_DataView == szToken2)
    {
      if (pAccepted)
        *pAccepted = inout_uiCurToken;

      inout_uiCurToken += 2;
      return true;
    }

    return false;
  }

  bool AcceptUnless(const TokenStream& tokens, ezUInt32& inout_uiCurToken, const char* szToken1, const char* szToken2, ezUInt32* pAccepted)
  {
    SkipWhitespace(tokens, inout_uiCurToken);

    if (inout_uiCurToken + 1 >= tokens.GetCount())
      return false;

    if (tokens[inout_uiCurToken]->m_DataView == szToken1 && tokens[inout_uiCurToken + 1]->m_DataView != szToken2)
    {
      if (pAccepted)
        *pAccepted = inout_uiCurToken;

      inout_uiCurToken += 1;
      return true;
    }

    return false;
  }

  void CombineRelevantTokensToString(const TokenStream& tokens, ezUInt32 uiCurToken, ezStringBuilder& out_sResult)
  {
    out_sResult.Clear();
    ezStringBuilder sTemp;

    for (ezUInt32 t = uiCurToken; t < tokens.GetCount(); ++t)
    {
      if ((tokens[t]->m_iType == ezTokenType::LineComment) || (tokens[t]->m_iType == ezTokenType::BlockComment) || (tokens[t]->m_iType == ezTokenType::Newline) || (tokens[t]->m_iType == ezTokenType::EndOfFile))
        continue;

      sTemp = tokens[t]->m_DataView;
      out_sResult.Append(sTemp.GetView());
    }
  }

  void CreateCleanTokenStream(const TokenStream& tokens, ezUInt32 uiCurToken, TokenStream& inout_destination, bool bKeepComments)
  {
    SkipWhitespace(tokens, uiCurToken);

    for (ezUInt32 t = uiCurToken; t < tokens.GetCount(); ++t)
    {
      if (tokens[t]->m_iType == ezTokenType::Newline)
      {
        // remove all whitespace before a newline
        while (!inout_destination.IsEmpty() && inout_destination.PeekBack()->m_iType == ezTokenType::Whitespace)
          inout_destination.PopBack();

        // if there is already a newline stored, discard the new one
        if (!inout_destination.IsEmpty() && inout_destination.PeekBack()->m_iType == ezTokenType::Newline)
          continue;
      }

      inout_destination.PushBack(tokens[t]);
    }
  }

  void CombineTokensToString(const TokenStream& tokens0, ezUInt32 uiCurToken, ezStringBuilder& out_sResult, bool bKeepComments, bool bRemoveRedundantWhitespace, bool bInsertLine)
  {
    TokenStream Tokens;

    if (bRemoveRedundantWhitespace)
    {
      CreateCleanTokenStream(tokens0, uiCurToken, Tokens, bKeepComments);
      uiCurToken = 0;
    }
    else
      Tokens = tokens0;

    out_sResult.Clear();
    ezStringBuilder sTemp;

    ezUInt32 uiCurLine = 0xFFFFFFFF;
    ezHashedString sCurFile;

    for (ezUInt32 t = uiCurToken; t < Tokens.GetCount(); ++t)
    {
      // skip all comments, if not desired
      if ((Tokens[t]->m_iType == ezTokenType::BlockComment || Tokens[t]->m_iType == ezTokenType::LineComment) && !bKeepComments)
        continue;

      if (Tokens[t]->m_iType == ezTokenType::EndOfFile)
        return;

      if (bInsertLine)
      {
        if (out_sResult.IsEmpty())
        {
          out_sResult.AppendFormat("#line {0} \"{1}\"\n", Tokens[t]->m_uiLine, Tokens[t]->m_File);
          uiCurLine = Tokens[t]->m_uiLine;
          sCurFile = Tokens[t]->m_File;
        }

        if (Tokens[t]->m_iType == ezTokenType::Newline)
        {
          ++uiCurLine;
        }

        if (t > 0 && Tokens[t - 1]->m_iType == ezTokenType::Newline)
        {
          if (Tokens[t]->m_uiLine != uiCurLine || Tokens[t]->m_File != sCurFile)
          {
            out_sResult.AppendFormat("\n#line {0} \"{1}\"\n", Tokens[t]->m_uiLine, Tokens[t]->m_File);
            uiCurLine = Tokens[t]->m_uiLine;
            sCurFile = Tokens[t]->m_File;
          }
        }
      }

      sTemp = Tokens[t]->m_DataView;
      out_sResult.Append(sTemp.GetView());
    }
  }
} // namespace ezTokenParseUtils

EZ_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Implementation_TokenParseUtils);
