#include <atomic>
#include <stdexcept>

template <typename T>
struct Node{
    T val;
    std::atomic_ref<Node*> next; // requires C++20

    Node(T x, Node* next = nullptr) : val(x), next(nullptr) {}
};

template <typename T>
class LockFreeQueue{
    
private:
    std::atomic_ref<Node*> head;
    std::atomic_ref<Node*> tail;
    
public:
    LockFreeQueue(){
        this->head = new Node(NULL);
        this->tail = new Node(NULL);
    }
    
    void enq(T x){ }
    
    void deq(T x){ }

    void print(){ }
};