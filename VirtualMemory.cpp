#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <cmath>

/**
 * clears all cells in the given frame index
 */
void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}


void VMinitialize() {
    clearTable(0);
}

/**
 * returns the absolute value of the subtraction of the given values
 */
uint64_t absSub(uint64_t p1, uint64_t p2){
    int res = (int)p1-(int)p2;
    if (res < 0){
        return p2-p1;
    }
    return p1-p2;
}


/**
 * helper function for getCurBits, to get specific bits by start and end indexes.
 */
uint64_t maskBits(uint64_t virtualAddress, unsigned int start_index, unsigned int end_index){

    uint64_t r = 0;
    for (unsigned int i=start_index; i<=end_index; i++)
        r |= 1 << i;

    r = r&virtualAddress;
    for (unsigned int j=0; j < start_index; j++){
        r = r >> 1;
    }

    return r;
}

/**
 * get bits of current section in virtualAddress
 */
uint64_t getCurBits(uint64_t virtualAddress, unsigned int counter){

    unsigned int len = VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH;
    unsigned int starter_len = len%OFFSET_WIDTH;
    unsigned int initial_start = VIRTUAL_ADDRESS_WIDTH-1;
    unsigned int start;
    unsigned int end;
    if (counter == 0){
        start = initial_start;
        if (starter_len == 0){
            end = start - OFFSET_WIDTH + 1;
        }
        else{
            end = start - starter_len + 1;
        }
    }
    else{
        if (starter_len == 0){
            start = initial_start - (counter * OFFSET_WIDTH);
        }else{
            start = initial_start- starter_len - ((counter-1) * OFFSET_WIDTH);
        }
        end = start - OFFSET_WIDTH + 1;
    }

    return maskBits(virtualAddress, end, start);
}


/**
 * updates most distant page, in a cyclic matter, from destination page, in order to be evicted (if needed).
 */
void updateMaxDistPage(uint64_t page_num, uint64_t &cur_p, uint64_t &maxP, word_t curr_frame,
                       word_t &maxPFrame, uint64_t &maxPFatherAddr, uint64_t curr_father_addr) {
    uint64_t cur_abs = absSub(page_num, cur_p);
    uint64_t max_abs = absSub(page_num, maxP);
    if(fmin((uint64_t )NUM_PAGES-cur_abs,cur_abs)>fmin((uint64_t)NUM_PAGES-max_abs,max_abs)){
        maxP = cur_p;
        maxPFrame = curr_frame;
        maxPFatherAddr = curr_father_addr;
    }
}


/**
 * runs through virtual tree in DFS manner, in order to find an available frame.
 */
void runDFS(int currDepth,word_t currFrame,word_t& freeFrame, word_t currFatherFrame, word_t& freeFrameFather, uint64_t currFatherAddress, uint64_t& prevFatherAddress, uint64_t& max_p_father_addr, bool &allZeros, int &maxFrameIndex,uint64_t restoredPage, uint64_t& max_dist_page,word_t &max_p_frame, uint64_t &max_p_father_frame, uint64_t page_num, word_t future_father_frame){
    uint64_t curPrevFatherAddr =0;
    if (currDepth == TABLES_DEPTH)
    {
        updateMaxDistPage(page_num, restoredPage, max_dist_page, currFrame, max_p_frame,
                          max_p_father_addr, currFatherAddress);
        return;
    }
    int numZeros = 0;
    if (currFrame!=0) {
        currFatherFrame = currFrame;
    }
    for (int i=0; i < PAGE_SIZE; i++){

        word_t temp_val;
        if (numZeros == 0) {
            curPrevFatherAddr = currFatherAddress;
        }
        currFatherAddress = (uint64_t)currFatherFrame * PAGE_SIZE + i;
        PMread(currFatherAddress, &temp_val);
        if (temp_val==0){
            numZeros++;
        }
        else {
            currFrame = temp_val;
            if (currFrame > maxFrameIndex) {
                maxFrameIndex = currFrame;
            }
            restoredPage = (restoredPage <<OFFSET_WIDTH) + i;
            runDFS(currDepth+1, currFrame, freeFrame, currFatherFrame, freeFrameFather,currFatherAddress,prevFatherAddress, max_p_father_addr, allZeros, maxFrameIndex, restoredPage, max_dist_page,max_p_frame,max_p_father_frame, page_num, future_father_frame);
            restoredPage = restoredPage >>OFFSET_WIDTH;
        }
    }
    if (numZeros==PAGE_SIZE && future_father_frame != currFrame)
    {
        allZeros = true;
        freeFrame = currFrame;
        freeFrameFather = currFatherFrame;
        prevFatherAddress = curPrevFatherAddr;
        return;
    }
}


/**
 * runs through virtual address and its corresponding frames, until reaching the offset.
 */
word_t runThroughAddrFrames(uint64_t virtualAddress){
    word_t addr2=0, currFrame=0, prevFatherFrame=0, maxPFrame=0, futureFatherFrame=0, freeFrame = 0, freeFrameFather = 0;
    uint64_t pageNum = maskBits(virtualAddress, OFFSET_WIDTH, VIRTUAL_ADDRESS_WIDTH - 1);
    uint64_t curBitsAddr = 0, max_dist_page = 0, maxPFatherAddr =0, cur_father_addr = 0,
    prevFatherAddr=0, restoredPage = 0;
    bool allZeros;
    int maxFrameIndex;
    for(int i=0; i< TABLES_DEPTH; ++i){
        curBitsAddr = getCurBits(virtualAddress, (unsigned int) i);
        uint64_t futureFatherAddr = futureFatherFrame*PAGE_SIZE + curBitsAddr;
        PMread(futureFatherAddr,&addr2);
        if (addr2==0){
            currFrame=0, cur_father_addr = 0, prevFatherAddr = 0, maxFrameIndex = 0;
            allZeros = false;
            restoredPage = 0, freeFrameFather = 0, prevFatherFrame =0;
            max_dist_page = pageNum;
            runDFS(0,currFrame,freeFrame, prevFatherFrame,freeFrameFather,cur_father_addr,prevFatherAddr, maxPFatherAddr, allZeros, maxFrameIndex, restoredPage, max_dist_page,maxPFrame, maxPFatherAddr, pageNum, futureFatherFrame);
            if (allZeros && futureFatherFrame != freeFrame){
                PMwrite(prevFatherAddr,0);
            }
            else if (maxFrameIndex + 1<NUM_FRAMES)
            {
                freeFrame = maxFrameIndex + 1;
            }
            else
            {
                PMwrite(maxPFatherAddr,0);
                PMevict((uint64_t )maxPFrame,max_dist_page);
                freeFrame = maxPFrame;
            }
            if (i == TABLES_DEPTH -1){
                PMrestore((uint64_t )freeFrame, pageNum);
            }else{
                clearTable((uint64_t )freeFrame);
            }

            PMwrite(futureFatherAddr,freeFrame);
            futureFatherFrame = freeFrame;
        }else {
            futureFatherFrame = addr2;
        }
    }
    return futureFatherFrame;
}


int VMread(uint64_t virtualAddress, word_t* value) {
    if (virtualAddress <= VIRTUAL_MEMORY_SIZE) {
        word_t frame = runThroughAddrFrames(virtualAddress);
        PMread((long long) frame * PAGE_SIZE + getCurBits(virtualAddress, TABLES_DEPTH), value);
        return 1;
    }
    return 0;
}


int VMwrite(uint64_t virtualAddress, word_t value) {
    if (virtualAddress <= VIRTUAL_MEMORY_SIZE) {
        word_t frame = runThroughAddrFrames(virtualAddress);
        PMwrite((long long) frame * PAGE_SIZE + getCurBits(virtualAddress, TABLES_DEPTH), value);
        return 1;
    }
    return 0;
}
