#if defined _easyjson_included
 #endinput
#endif
#define _easyjson_included

enum JSONType
{
	Type_String,
	Type_Integer,
	Type_Float,
	Type_Object,
	Type_Boolean
}

stock Handle:CreateJSON()
{
	new Handle:m_hJSON = CreateTrie();
	new Handle:m_hJSONKeys = CreateArray(512);
	SetTrieValue(m_hJSON, "json_key_array", m_hJSONKeys);
	return m_hJSON;
}

stock Handle:DecodeJSON(const String:json[], &len=0)
{
	decl m_iPos, m_iIgnore;
	new m_iLength = strlen(json);

	m_iPos = JSONIgnore(json);
	if(m_iPos == -1 || json[m_iPos] != '{')
		return INVALID_HANDLE;
	++m_iPos;
	m_iIgnore = JSONIgnore(json[m_iPos]);
	if(m_iIgnore == -1)
		return INVALID_HANDLE;
	m_iPos += m_iIgnore;
	if(json[m_iPos] == '}')
		return INVALID_HANDLE;

	decl m_iStringStart;
	decl m_iStringEnd;

	new Handle:m_hJSON = CreateJSON();
	while(m_iPos<m_iLength && json[m_iPos] != '}')
	{
		if(GetJSONString(json[m_iPos], m_iStringStart, m_iStringEnd))
		{
			new String:m_szKey[m_iStringEnd-m_iStringStart+1];
			strcopy(m_szKey, m_iStringEnd-m_iStringStart+1, json[m_iPos+m_iStringStart]);
			m_iPos += m_iStringEnd+1;
			m_iPos += JSONIgnore(json[m_iPos]);
			if(json[m_iPos] == '{')
			{
				new m_iLen = -1;
				new Handle:m_hObject = DecodeJSON(json[m_iPos], m_iLen);
				if(m_hObject != INVALID_HANDLE)
				{
					JSONSetObject(m_hJSON, m_szKey, m_hObject);
					m_iPos += m_iLen;
				}
				else
				{
					DestroyJSON(m_hJSON);
					return INVALID_HANDLE;
				}
			} else if(GetJSONString(json[m_iPos], m_iStringStart, m_iStringEnd))
			{
				new String:m_szValue[m_iStringEnd-m_iStringStart+1];
				strcopy(m_szValue, m_iStringEnd-m_iStringStart+1, json[m_iPos+m_iStringStart]);
				m_iPos += m_iStringEnd+1;
				m_iPos += JSONIgnore(json[m_iPos]);

				new m_iDot = 0;

				if(m_iStringStart != 0)
					JSONSetString(m_hJSON, m_szKey, m_szValue);
				else if(strcmp(m_szValue, "true")==0)
					JSONSetBoolean(m_hJSON, m_szKey, true);
				else if(strcmp(m_szValue, "false")==0)
					JSONSetBoolean(m_hJSON, m_szKey, false);
				else if(JSONIsNumeric(m_szValue))
					JSONSetInteger(m_hJSON, m_szKey, StringToInt(m_szValue));
				else if((m_iDot = FindCharInString(m_szValue, '.'))!=-1)
				{
					m_szValue[m_iDot] = 0;
					if(JSONIsNumeric(m_szValue) && JSONIsNumeric(m_szValue[m_iDot+1]))
					{
						m_szValue[m_iDot] = '.';
						JSONSetFloat(m_hJSON, m_szKey, StringToFloat(m_szValue));
					}
				}
			}
		}
	}
	if(len != 0)
		len = m_iPos;

	return m_hJSON;
}

stock EncodeJSON(&Handle:json, String:output[], maxlen, bool:beautify=true, &len=0, tabs=1)
{
	new Handle:m_hKeyArray = INVALID_HANDLE;
	if(!GetTrieValue(json, "json_key_array", m_hKeyArray))
		return;

	new m_iKeys = GetArraySize(m_hKeyArray);

	decl String:m_szKey[512];
	decl Handle:m_hObject;

	if(2 >= maxlen)
		return;

	new String:m_szTabs[tabs+1];
	if(beautify)
		for(new i=0;i<tabs;++i)
			m_szTabs[i] = '\t';

	new m_iPos = 2;
	output[0] = '{';
	output[1] = '\n';
	for(new i=0;i<m_iKeys;++i)
	{
		GetArrayString(m_hKeyArray, i, STRING(m_szKey));
		if(!GetTrieValue(json, m_szKey, m_hObject))
			continue;
		decl JSONType:m_eType;
		GetTrieValue(m_hObject, "type", m_eType);
		switch(m_eType)
		{
			case Type_String:
			{
				decl m_iLength;
				GetTrieValue(m_hObject, "size", m_iLength);
				decl String:m_szValue[m_iLength*2+1];
				GetTrieString(m_hObject, "value", m_szValue, m_iLength+1);
				ReplaceString(m_szValue, m_iLength*2+1, "\"", "\\\"");
				m_iPos += Format(output[m_iPos], maxlen-m_iPos, "%s\"%s\": \"%s\"", m_szTabs, m_szKey, m_szValue);
			}

			case Type_Boolean:
			{
				decl bool:m_bValue;
				GetTrieValue(m_hObject, "value", m_bValue);
				if(m_bValue)
					m_iPos += Format(output[m_iPos], maxlen-m_iPos, "%s\"%s\": true", m_szTabs, m_szKey);
				else
					m_iPos += Format(output[m_iPos], maxlen-m_iPos, "%s\"%s\": false", m_szTabs, m_szKey);
			}

			case Type_Integer:
			{
				decl m_iValue;
				GetTrieValue(m_hObject, "value", m_iValue);
				m_iPos += Format(output[m_iPos], maxlen-m_iPos, "%s\"%s\": %d", m_szTabs, m_szKey, m_iValue);
			}

			case Type_Float:
			{
				decl Float:m_fValue;
				GetTrieValue(m_hObject, "value", m_fValue);
				m_iPos += Format(output[m_iPos], maxlen-m_iPos, "%s\"%s\": %f", m_szTabs, m_szKey, m_fValue);
			}

			case Type_Object:
			{
				decl Handle:m_hValue;
				GetTrieValue(m_hObject, "value", m_hValue);
				m_iPos += Format(output[m_iPos], maxlen-m_iPos, "%s\"%s\": ", m_szTabs, m_szKey);

				new m_iLen = -1;
				EncodeJSON(m_hValue, output[m_iPos], maxlen-m_iPos, beautify, m_iLen, tabs+1);
				m_iPos += m_iLen;
			}
		}
		if(m_iPos >= maxlen)
			return;
		if(m_iKeys != i+1)
		{
			output[m_iPos] = ',';
			++m_iPos;
		}
		if(m_iPos >= maxlen)
			return;
		if(beautify)
			output[m_iPos] = '\n';
		++m_iPos;
	}
	if(m_iPos < maxlen)
	{
		m_szTabs[tabs-1] = 0;
		m_iPos += Format(output[m_iPos], maxlen-m_iPos, "%s}", m_szTabs);
	}
	if(len != 0)
		len=m_iPos;
}

stock JSONSetString(&Handle:json, const String:key[], const String:value[])
{
	new Handle:m_hKeyArray = INVALID_HANDLE;
	if(!GetTrieValue(json, "json_key_array", m_hKeyArray))
		return;
	PushArrayString(m_hKeyArray, key);

	new Handle:m_hObject = CreateTrie();
	SetTrieString(m_hObject, "value", value);
	SetTrieValue(m_hObject, "type", Type_String);
	SetTrieValue(m_hObject, "size", strlen(value));
	SetTrieValue(json, key, m_hObject);
}

stock JSONSetBoolean(&Handle:json, const String:key[], bool:value)
{
	new Handle:m_hKeyArray = INVALID_HANDLE;
	if(!GetTrieValue(json, "json_key_array", m_hKeyArray))
		return;
	PushArrayString(m_hKeyArray, key);

	new Handle:m_hObject = CreateTrie();
	SetTrieValue(m_hObject, "value", value);
	SetTrieValue(m_hObject, "type", Type_Boolean);
	SetTrieValue(m_hObject, "size", sizeof(value));
	SetTrieValue(json, key, m_hObject);
}

stock JSONSetInteger(&Handle:json, const String:key[], value)
{
	new Handle:m_hKeyArray = INVALID_HANDLE;
	if(!GetTrieValue(json, "json_key_array", m_hKeyArray))
		return;
	PushArrayString(m_hKeyArray, key);

	new Handle:m_hObject = CreateTrie();
	SetTrieValue(m_hObject, "value", value);
	SetTrieValue(m_hObject, "type", Type_Integer);
	SetTrieValue(m_hObject, "size", sizeof(value));
	SetTrieValue(json, key, m_hObject);
}

stock JSONSetFloat(&Handle:json, const String:key[], Float:value)
{
	new Handle:m_hKeyArray = INVALID_HANDLE;
	if(!GetTrieValue(json, "json_key_array", m_hKeyArray))
		return;
	PushArrayString(m_hKeyArray, key);

	new Handle:m_hObject = CreateTrie();
	SetTrieValue(m_hObject, "value", value);
	SetTrieValue(m_hObject, "type", Type_Float);
	SetTrieValue(m_hObject, "size", sizeof(value));
	SetTrieValue(json, key, m_hObject);
}

stock JSONSetObject(&Handle:json, const String:key[], Handle:value)
{
	new Handle:m_hKeyArray = INVALID_HANDLE;
	if(!GetTrieValue(json, "json_key_array", m_hKeyArray))
		return;
	PushArrayString(m_hKeyArray, key);

	new Handle:m_hObject = CreateTrie();
	SetTrieValue(m_hObject, "value", value);
	SetTrieValue(m_hObject, "type", Type_Object);
	SetTrieValue(m_hObject, "size", sizeof(value));
	SetTrieValue(json, key, m_hObject);
}

stock bool:JSONGetString(&Handle:json, const String:key[], String:out[], maxlen)
{
	decl Handle:m_hObject;
	if(!GetTrieValue(json, key, m_hObject))
		return false;
	decl JSONType:m_eType;
	GetTrieValue(m_hObject, "type", m_eType);
	if(m_eType != Type_String)
		return false;
	GetTrieString(m_hObject, "value", out, maxlen);
	return true;
}

stock bool:JSONGetObject(&Handle:json, const String:key[], &Handle:out)
{
	decl Handle:m_hObject;
	if(!GetTrieValue(json, key, m_hObject))
		return false;
	decl JSONType:m_eType;
	GetTrieValue(m_hObject, "type", m_eType);
	if(m_eType != Type_Object)
		return false;
	GetTrieValue(m_hObject, "value", out);
	return true;
}

stock bool:JSONGetBoolean(&Handle:json, const String:key[], &bool:value)
{
	decl Handle:m_hObject;
	if(!GetTrieValue(json, key, m_hObject))
		return false;
	decl JSONType:m_eType;
	GetTrieValue(m_hObject, "type", m_eType);
	if(m_eType != Type_Boolean)
		return false;
	GetTrieValue(m_hObject, "value", value);
	return true;
}

stock bool:JSONGetInteger(&Handle:json, const String:key[], &value)
{
	decl Handle:m_hObject;
	if(!GetTrieValue(json, key, m_hObject))
		return false;
	decl JSONType:m_eType;
	GetTrieValue(m_hObject, "type", m_eType);
	if(m_eType != Type_Integer)
		return false;
	GetTrieValue(m_hObject, "value", value);
	return true;
}

stock bool:JSONGetFloat(&Handle:json, const String:key[], &Float:value)
{
	decl Handle:m_hObject;
	if(!GetTrieValue(json, key, m_hObject))
		return false;
	decl JSONType:m_eType;
	GetTrieValue(m_hObject, "type", m_eType);
	if(m_eType != Type_Float)
		return false;
	GetTrieValue(m_hObject, "value", value);
	return true;
}

stock JSONIgnore(const String:json[])
{
	new m_iLength = strlen(json);
	for(new i=0;i<m_iLength;++i)
		if(json[i] != ' ' && json[i] != '\t' && json[i] != ':' && json[i] != '\n' && json[i] != '\r' && json[i] != ',')
			return i;
	return -1;
}

stock JSONIsNumeric(const String:string[])
{
	new m_iLength = strlen(string);
	for(new i=0;i<m_iLength;++i)
		if(!(48<=string[i]<=57))
			return false;
	return true;
}

stock bool:GetJSONString(const String:json[], &start, &end)
{
	decl m_iCharPos;

	new m_iPos = JSONIgnore(json);
	if(json[m_iPos] == '"')
		++m_iPos;
	start = m_iPos;
	if(json[m_iPos-1] == '"')
	{
		while((m_iCharPos = FindCharInString(json[m_iPos], '"')) != -1)
		{
			m_iPos += m_iCharPos;
			if(json[m_iPos-1] == '\\')
				continue;
			end = m_iPos;
			return true;
		}
	}
	m_iCharPos = FindCharInString(json[m_iPos], ',');
	if(m_iCharPos != -1)
	{
		end = m_iPos+m_iCharPos;
		return true;
	}
	else
	{
		new m_iLength = strlen(json);
		for(;m_iPos<m_iLength;++m_iPos)
		{
			if(!(48<=json[m_iPos]<=57) && !(97<=json[m_iPos]<=122))
			{
				end = m_iPos;
				return true;
			}
		}
	}
	return false;
}

stock DestroyJSON(&Handle:json)
{
	new Handle:m_hKeyArray = INVALID_HANDLE;
	if(!GetTrieValue(json, "json_key_array", m_hKeyArray))
	{
		CloseHandle(json);
		return;
	}

	new m_iKeys = GetArraySize(m_hKeyArray);
	decl String:m_szKey[512];
	decl Handle:m_hObject;
	for(new i=0;i<m_iKeys;++i)
	{
		GetArrayString(m_hKeyArray, i, STRING(m_szKey));
		if(!GetTrieValue(json, m_szKey, m_hObject))
			continue;
		decl String:m_szType[32];
		GetTrieString(m_hObject, "type", STRING(m_szType));
		if(strcmp(m_szType, "object")==0)
		{
			new Handle:m_hValue;
			GetTrieValue(m_hObject, "value", m_hValue);
			DestroyJSON(m_hValue);
		}
	}

	CloseHandle(m_hKeyArray);
	CloseHandle(json);
}