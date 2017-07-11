#pragma once

#include <list>
#include <memory>
#include <iostream>
#include <iomanip>
#include <fcntl.h>  
#include <io.h>  

template <typename S>
struct GenericTreeNode
{
	GenericTreeNode() = delete;

	GenericTreeNode(S& _data)
		:
		data(_data)
		, parent(nullptr)
		, level(0)
	{
	}

	GenericTreeNode(GenericTreeNode* _parent, S& _data)
		:
		data(_data)
		, parent(_parent)
	{
		level = (parent) ? parent->level+1 : 0;
	}

	GenericTreeNode(const GenericTreeNode& other)
		: data(other.data)
		, listChildren(std::move(other.listChildren))
		, parent(other.parent)
		, level(other.level)
	{
	}

	GenericTreeNode& operator = (const GenericTreeNode& rhs)
	{
		if (this != &rhs)
		{
			data			= rhs.data;
			listChildren	= std::move(rhs.listChildren);
			level			= rhs.level;
		}

		return *this;
	}

	GenericTreeNode(GenericTreeNode&& other)
	{
		listChildren.empty();

		data			= other.data;
		parent			= other.parent;
		listChildren	= std::move(other.listChildren);
		level			= other.level;

		other.parent = nullptr;
		other.listChildren.clear();
		other.level = 0;
	}

	GenericTreeNode& operator=(GenericTreeNode&& other)
	{
		if (this != &other)
		{
			listChildren.empty();

			data		 = other.data;
			parent		 = other.parent;
			listChildren = std::move(other.listChildren);
			level		 = other.level;
			 
			other.data		= 0;
			other.parent	= nullptr;
			other.listChildren.clear();
			other.level		= 0;
		}

		return *this;
	}


	S				data;
	short				level;
	GenericTreeNode<S>*	parent;

	std::list<GenericTreeNode<S>*>	listChildren;
};

template <typename T>
class generic_tree
{
public:

	generic_tree(GenericTreeNode<T>* _parent, T& root_value)
	{
		ptrRoot = _new_node(_parent, root_value);
	}

	~generic_tree()
	{
		_tree_cleaner(ptrRoot);
	}

	GenericTreeNode<T>* add(GenericTreeNode<T>* _parent, T& _data)
	{
		GenericTreeNode<T> *pnew = _new_node(_parent, _data);
		_parent->listChildren.push_back(pnew);

		return pnew;
	}

	GenericTreeNode<T>* get_root() const { return ptrRoot; }

protected:

	GenericTreeNode<T>* _new_node(GenericTreeNode<T>* _parent, T& _data)
	{
		return new GenericTreeNode<T>(_parent, _data);
	}

	void _tree_cleaner(GenericTreeNode<T> *node)
	{
		if (!node) return;

		for (auto &ob : node->listChildren)
			_tree_cleaner(ob);

		delete node;
	}

protected:
	GenericTreeNode<T> * ptrRoot;
};