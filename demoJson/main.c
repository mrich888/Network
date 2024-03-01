#include <json-c/json.h>
#include <stdio.h>
#include <string.h>

#define MAX_TYPE      20
#define MAX_USERNAME  20
#define MAX_PASSWORD  10
#define MAX_CONTENT   1024
#define MAX_TARGET    10


int main()
{
    /*todo....
    指针，需要判空
    考虑到资源问题，使用do ... while ... 先创建在判断，出问题及时释放资源
    还要考虑怎么释放，从哪里开始释放
    */

    /* 第一层 */
    struct json_object *bigJ = json_object_new_object();

    /* name和alexa分别创建对象并添加到bigJ中 */
    struct json_object *name = json_object_new_string("runoob");
    struct json_object *alexa = json_object_new_int64(10000);
    json_object_object_add(bigJ, "name", name);
    json_object_object_add(bigJ, "alexa", alexa);

    /* 创建shopping数组对象 */
    struct json_object *shopping = json_object_new_array();
    /* 创建数组中每一个字符串的对象 */
    struct json_object *val1 = json_object_new_string("jingdong");
    struct json_object *val2 = json_object_new_string("pinduoduo");
    struct json_object *val3 = json_object_new_string("taobao");
    /* 将每一个对象放到数组里 */
    json_object_array_add(shopping, val1);
    json_object_array_add(shopping, val2);
    json_object_array_add(shopping, val3);
    /* 将数组放到bigJ中 */
    json_object_object_add(bigJ, "shopping", shopping);

    /* 创建sites对象 */
    struct json_object *sites = json_object_new_object();
    /* 创建sites中每一个对象 */
    struct json_object *site1 = json_object_new_string("www.runoob.com");
    struct json_object *site2 = json_object_new_string("m.runoob.com");
    /* 先将sites中的对象插进去 */
    json_object_object_add(sites, "site1", site1);
    json_object_object_add(sites, "site2", site2);
    /* 再将sites插入到big中 */
    json_object_object_add(bigJ, "sites", sites);

    const char * ptr = json_object_to_json_string(bigJ);
    printf("bigJ:%s\n", ptr);



    /* 将字符串转化成json对象 */
#if 0

    struct json_object * ageObj = json_tokener_parse(str);

    struct json_object * keyValue = json_object_object_get(ageObj, key);
    if (keyValue == NULL)
    {
        printf("fergregrt\n");
    }

    int64_t val = json_object_get_int64(keyValue);
    printf("value:%ld\n", val);

        printf("=====================demo server=======================\n");
    /* 将字符串转化成json对象 */
#if 0
    // struct json_object * ageObj = json_object_new_string(str);
#else
    struct json_object * newJsonObj = json_tokener_parse(str);
#endif

    struct json_object * wayVal = json_object_object_get(newJsonObj, "way");
    if (wayVal == NULL)
    {
        printf("get wayValue error\n");
        exit(-1);
    }

    /* 解析int类型的json对象 */
    printf("way:%ld\n", json_object_get_int64(wayVal));


    struct json_object * nameVal = json_object_object_get(newJsonObj, "name");
    if (nameVal == NULL)
    {
        perror("get nameVal error\n");
        exit(-1);
    }

    printf("name:%s\n", json_object_get_string(nameVal));


    /* 释放json对象 */
    json_object_put(jsonObj);
#endif


    return 0;
}

