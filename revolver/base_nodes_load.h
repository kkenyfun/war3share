/*************************************************************************************
*filename:	base_nodes_load.h
*
*to do:		��������ڵ㸺�ع�����ѡȡ�㷨
*Create on: 2012-04
*Author:	zerok
*check list:��Ҫ�����������ѡȡ�㷨����ͨ��2�ֲ��ҷ�����ѡȡ
*************************************************************************************/
#ifndef __BASE_NODES_LOAD_H
#define __BASE_NODES_LOAD_H

#include "base_namespace.h"
#include "base_typedef.h"

#include <map>
#include <set>
#include <vector>

using namespace std;

BASE_NAMESPACE_BEGIN_DECL

const int32_t MAX_LOAD_VALUE = 95;

typedef set<uint32_t>	SERVER_ID_SET;
typedef vector<uint32_t> SERVER_ID_ARRAY;

typedef struct NodeLoadInfo
{
	uint32_t	node_id;		//�ڵ�ID
	uint16_t	node_load;		//�ڵ㸺��ֵ��0��100,100��ʾ�������

	NodeLoadInfo()
	{
		node_id = 0;
		node_load = 100;
	};
}NodeLoadInfo;



typedef map<uint32_t, NodeLoadInfo>	NodeLoadInfoMap;

typedef struct NodeRange
{
	int32_t		min_value;
	int32_t		max_value;
	uint32_t	node_id;

	NodeRange()
	{
		min_value = 0;
		max_value = 0;
		node_id = 0;
	};
}NodeRange;

typedef vector<NodeRange>	NodeRangeArray;

class CNodeLoadManager
{
public:
	CNodeLoadManager();
	~CNodeLoadManager();

	void			add_node(const NodeLoadInfo& info);
	void			update_node(const NodeLoadInfo& info);
	void			del_node(uint32_t node_id);

	uint32_t		select_node();
	uint32_t		select_node(const SERVER_ID_SET& ids);
	bool			select_node(NodeLoadInfo& info);
	uint32_t		size() const {return node_info_map_.size();};
private:
	uint32_t		locate_server(int32_t region);

private:
	NodeLoadInfoMap	node_info_map_;		//�ڵ㸺�ر�
	NodeRangeArray	node_ranges_;		//һ�����ڵĸ��������
	
	bool			create_range_;		//�Ƿ���Ҫ�ؽ�����ѡȡ��
	int32_t			region_;			//����ȫ����
};

BASE_NAMESPACE_END_DECL
#endif

/************************************************************************************/
