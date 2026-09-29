#include "page.h"
#include "dgus_tools.h"
#include "log.h"

s_page_t s_page;

void page_0_init(void)
{

}

void page_0_deal(void)
{

}

void page_0_finish(void)
{

}

s_page_deal_t page_deal[PAGE_NUM] = 
{
    // {init是否处理, deal是否处理, finish是否处理, init函数指针, deal函数指针, finish函数指针},
    {PAGE_ON, PAGE_ON, PAGE_ON, page_0_init, page_0_deal, page_0_finish}, // 页0处理函数
    {PAGE_OFF, PAGE_OFF, PAGE_OFF, NULL, NULL, NULL}, // 页1处理函数
};

void page_deal_init(void)
{
    s_page.page_last = 0;
    s_page.page_now = 0;
}

void page_deal_func(void)
{
    u16 page_id;

    page_id = Read_Vp(0x0014);

    if(s_page.page_now  != page_id)
    {
        s_page.page_last = s_page.page_now;
        
        s_page.page_now = page_id;
        
        LOG_DEBUG("page change %hu --> %hu\n", s_page.page_last, s_page.page_now );

        if(s_page.page_last  < PAGE_NUM)
        {
            if(page_deal[s_page.page_last].page_finish_is_enable == PAGE_ON)
            {
                if(page_deal[s_page.page_last].page_finish!= NULL)
                {
                    page_deal[s_page.page_last].page_finish();
                    LOG_DEBUG("page_finish %hu\n", s_page.page_last);
                }
            }
        }
        if(s_page.page_now < PAGE_NUM)
        {

            if(page_deal[s_page.page_now].page_init_is_enable == PAGE_ON)
            {
                if(page_deal[s_page.page_now].page_init!= NULL)
                {
                    page_deal[s_page.page_now].page_init();
                    LOG_DEBUG("page_init %hu\n", s_page.page_now);
                }
            }
        }
    }
    else
    {
        if(s_page.page_now < PAGE_NUM)
        {
            if(page_deal[s_page.page_now ].page_deal_is_enable == PAGE_ON)
            {
                if(page_deal[s_page.page_now ].page_deal != NULL)
                {
                    page_deal[s_page.page_now ].page_deal();
                }
            }
        }
    }
}
