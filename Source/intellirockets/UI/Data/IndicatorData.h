#pragma once

#include "CoreMinimal.h"
#include "IndicatorData.generated.h"

USTRUCT(BlueprintType)
struct FIndicatorInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString NameEn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> AlgorithmNames;  // 关联的算法名称列表

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> PrototypeNames;  // 关联的分系统名称列表

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Level;  // "algorithm" 或 "system"，表示算法级还是系统级

	FIndicatorInfo()
		: Id(TEXT(""))
		, Name(TEXT(""))
		, NameEn(TEXT(""))
		, Description(TEXT(""))
		, Level(TEXT(""))
	{
	}
};

USTRUCT(BlueprintType)
struct FIndicatorSubCategory
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FIndicatorInfo> Indicators;

	FIndicatorSubCategory()
		: Id(TEXT(""))
		, Name(TEXT(""))
	{
	}
};

USTRUCT(BlueprintType)
struct FIndicatorCategory
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FIndicatorSubCategory> SubCategories;

	FIndicatorCategory()
		: Id(TEXT(""))
		, Name(TEXT(""))
	{
	}
};

USTRUCT(BlueprintType)
struct FIndicatorData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FIndicatorCategory> Categories;

	FIndicatorData()
	{
	}
};

/**
 * 指标数据加载器（从JSON文件加载）
 */
class FIndicatorDataLoader
{
public:
	static bool LoadFromFile(const FString& FilePath, FIndicatorData& OutData);
	static bool LoadFromString(const FString& JsonString, FIndicatorData& OutData);
	static bool SaveToFile(const FString& FilePath, const FIndicatorData& Data);

private:
	static FIndicatorInfo ParseIndicator(const TSharedPtr<FJsonObject>& JsonObj);
	static FIndicatorSubCategory ParseSubCategory(const TSharedPtr<FJsonObject>& JsonObj);
	static FIndicatorCategory ParseCategory(const TSharedPtr<FJsonObject>& JsonObj);
};
