#pragma once
#define CLIST_HPP

template <typename Type> class List
{
  public:
    /**
     * Construct an empty list.
     */
    List()
    {
        coreNode = NULL;
    }

    /**
     * Destroy the list. The contained nodes are not freed here.
     */
    ~List() {}

    /**
     * Append a value to the end of the list.
     *
     * @param value    Element to append.
     */
    void
    Add(Type *value)
    {
        NODE *tempNode;

        if (coreNode == NULL)
        {
            coreNode = new NODE;
            coreNode->data = value;
            coreNode->link = NULL;
        }
        else
        {
            tempNode = coreNode;
            while (tempNode->link != NULL)
                tempNode = tempNode->link;

            NODE *newNode = new NODE;
            newNode->data = value;
            newNode->link = NULL;
            tempNode->link = newNode;
        }
    }

    /**
     * Insert a value at the head of the list.
     *
     * @param value    Element to insert.
     */
    void
    AddToTop(Type *value)
    {
        NODE *q = new NODE;
        q->data = value;
        q->link = coreNode;
        coreNode = q;
    }

    /**
     * Insert a value after the element at the given index.
     *
     * @param c        Zero-based index of the element to insert after.
     * @param value    Element to insert.
     */
    void
    Insert(int c, void *value)
    {
        NODE *q, *t;
        for (int i = 0, q = coreNode; i < c; i++)
        {
            q = q->link;
            if (q == NULL)
                return;
        }

        t = new NODE;
        t->data = value;
        t->link = q->link;
        q->link = t;
    }

    /**
     * Unlink and delete the node holding the given value.
     *
     * @param value    Element to remove.
     *
     * @return The removed value, or NULL if it was not found.
     */
    Type *
    Remove(Type *value)
    {
        NODE *q, *r;
        q = coreNode;
        if (q->data == value)
        {
            coreNode = coreNode->link;
            Type *val = q->data;
            delete (q);
            return val;
        }

        r = q;
        while (q != NULL)
        {
            if (q->data == value)
            {
                r->link = q->link;
                Type *val = q->data;
                delete (q);
                return val;
            }

            r = q;
            q = q->link;
        }
        return NULL;
    }

    /**
     * Unlink the node at the given index.
     *
     * @param index    Zero-based index of the node to remove.
     *
     * @return The value at that index, or NULL if the index is out of range.
     */
    Type *
    RemoveAt(int index)
    {
        Type *result = NULL;
        NODE *lastNode, *tempNode;
        if (index == 0)
        {
            result = coreNode->data;
            coreNode = coreNode->link;
            delete (coreNode);
            return result;
        }
        else
        {
            lastNode = coreNode->link;
            tempNode = coreNode;
            int currentIndex = 1;
            while (lastNode != NULL)
            {
                if (currentIndex == index)
                {
                    tempNode->link = lastNode->link;
                    result = lastNode->data;
                    delete (lastNode);
                    return result;
                }
                tempNode = lastNode;
                lastNode = lastNode->link;
                currentIndex++;
            }
        }
        return result;
    }

    /**
     * Return the value at the given index.
     *
     * @param index    Zero-based index of the element.
     *
     * @return The value at that index, or NULL if the index is out of range.
     */
    Type *
    Get(int index)
    {
        if (index == 0)
        {
            return coreNode->data;
        }
        else if (coreNode)
        {
            NODE *lastNode = coreNode->link;
            int currentIndex = 1;
            while (lastNode != NULL)
            {
                if (currentIndex == index)
                {
                    return lastNode->data;
                }

                lastNode = lastNode->link;
                currentIndex++;
            }
        }
        return NULL;
    }

    /**
     * Count the elements in the list.
     *
     * @return The number of elements.
     */
    int
    Count()
    {
        NODE *q;
        int c = 0;
        for (q = coreNode; q != NULL; q = q->link)
            c++;

        return c;
    }

  private:
    struct NODE
    {
        Type *data;
        NODE *link;
    } *coreNode;
};
