#include "UI/Data/IndicatorData.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool FIndicatorDataLoader::LoadFromFile(const FString& FilePath, FIndicatorData& OutData)
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load indicator file: %s"), *FilePath);
		return false;
	}

	return LoadFromString(JsonString, OutData);
}

bool FIndicatorDataLoader::LoadFromString(const FString& JsonString, FIndicatorData& OutData)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON"));
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* CategoriesArray;
	if (!JsonObject->TryGetArrayField(TEXT("categories"), CategoriesArray))
	{
		UE_LOG(LogTemp, Error, TEXT("Missing 'categories' field"));
		return false;
	}

	OutData.Categories.Empty();
	for (const TSharedPtr<FJsonValue>& CategoryValue : *CategoriesArray)
	{
		if (CategoryValue->Type != EJson::Object) continue;
		OutData.Categories.Add(ParseCategory(CategoryValue->AsObject()));
	}

	return true;
}

bool FIndicatorDataLoader::SaveToFile(const FString& FilePath, const FIndicatorData& Data)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> CategoriesArray;

	for (const FIndicatorCategory& Category : Data.Categories)
	{
		TSharedPtr<FJsonObject> CategoryObj = MakeShareable(new FJsonObject);
		CategoryObj->SetStringField(TEXT("id"), Category.Id);
		CategoryObj->SetStringField(TEXT("name"), Category.Name);

		TArray<TSharedPtr<FJsonValue>> SubCategoriesArray;
		for (const FIndicatorSubCategory& SubCat : Category.SubCategories)
		{
			TSharedPtr<FJsonObject> SubCatObj = MakeShareable(new FJsonObject);
			SubCatObj->SetStringField(TEXT("id"), SubCat.Id);
			SubCatObj->SetStringField(TEXT("name"), SubCat.Name);

			TArray<TSharedPtr<FJsonValue>> IndicatorsArray;
			for (const FIndicatorInfo& Indicator : SubCat.Indicators)
			{
				TSharedPtr<FJsonObject> IndObj = MakeShareable(new FJsonObject);
				IndObj->SetStringField(TEXT("id"), Indicator.Id);
				IndObj->SetStringField(TEXT("name"), Indicator.Name);
				IndObj->SetStringField(TEXT("nameEn"), Indicator.NameEn);
				IndObj->SetStringField(TEXT("description"), Indicator.Description);
				IndObj->SetStringField(TEXT("level"), Indicator.Level);
				
				// 保存算法名称数组
				if (Indicator.AlgorithmNames.Num() > 0)
				{
					TArray<TSharedPtr<FJsonValue>> AlgorithmNamesArray;
					for (const FString& Name : Indicator.AlgorithmNames)
					{
						AlgorithmNamesArray.Add(MakeShareable(new FJsonValueString(Name)));
					}
					IndObj->SetArrayField(TEXT("algorithmNames"), AlgorithmNamesArray);
				}
				
				// 保存分系统名称数组
				if (Indicator.PrototypeNames.Num() > 0)
				{
					TArray<TSharedPtr<FJsonValue>> PrototypeNamesArray;
					for (const FString& Name : Indicator.PrototypeNames)
					{
						PrototypeNamesArray.Add(MakeShareable(new FJsonValueString(Name)));
					}
					IndObj->SetArrayField(TEXT("prototypeNames"), PrototypeNamesArray);
				}
				
				IndicatorsArray.Add(MakeShareable(new FJsonValueObject(IndObj)));
			}
			SubCatObj->SetArrayField(TEXT("indicators"), IndicatorsArray);
			SubCategoriesArray.Add(MakeShareable(new FJsonValueObject(SubCatObj)));
		}
		CategoryObj->SetArrayField(TEXT("subCategories"), SubCategoriesArray);
		CategoriesArray.Add(MakeShareable(new FJsonValueObject(CategoryObj)));
	}

	JsonObject->SetArrayField(TEXT("categories"), CategoriesArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	return FFileHelper::SaveStringToFile(OutputString, *FilePath);
}

FIndicatorInfo FIndicatorDataLoader::ParseIndicator(const TSharedPtr<FJsonObject>& JsonObj)
{
	FIndicatorInfo Info;
	JsonObj->TryGetStringField(TEXT("id"), Info.Id);
	JsonObj->TryGetStringField(TEXT("name"), Info.Name);
	JsonObj->TryGetStringField(TEXT("nameEn"), Info.NameEn);
	JsonObj->TryGetStringField(TEXT("description"), Info.Description);
	JsonObj->TryGetStringField(TEXT("level"), Info.Level);
	
	// 解析算法名称数组
	const TArray<TSharedPtr<FJsonValue>>* AlgorithmNamesArray;
	if (JsonObj->TryGetArrayField(TEXT("algorithmNames"), AlgorithmNamesArray))
	{
		Info.AlgorithmNames.Empty();
		for (const TSharedPtr<FJsonValue>& Value : *AlgorithmNamesArray)
		{
			if (Value->Type == EJson::String)
			{
				Info.AlgorithmNames.Add(Value->AsString());
			}
		}
	}
	
	// 解析分系统名称数组
	const TArray<TSharedPtr<FJsonValue>>* PrototypeNamesArray;
	if (JsonObj->TryGetArrayField(TEXT("prototypeNames"), PrototypeNamesArray))
	{
		Info.PrototypeNames.Empty();
		for (const TSharedPtr<FJsonValue>& Value : *PrototypeNamesArray)
		{
			if (Value->Type == EJson::String)
			{
				Info.PrototypeNames.Add(Value->AsString());
			}
		}
	}
	
	return Info;
}

FIndicatorSubCategory FIndicatorDataLoader::ParseSubCategory(const TSharedPtr<FJsonObject>& JsonObj)
{
	FIndicatorSubCategory SubCat;
	JsonObj->TryGetStringField(TEXT("id"), SubCat.Id);
	JsonObj->TryGetStringField(TEXT("name"), SubCat.Name);

	const TArray<TSharedPtr<FJsonValue>>* IndicatorsArray;
	if (JsonObj->TryGetArrayField(TEXT("indicators"), IndicatorsArray))
	{
		SubCat.Indicators.Empty();
		for (const TSharedPtr<FJsonValue>& IndicatorValue : *IndicatorsArray)
		{
			if (IndicatorValue->Type == EJson::Object)
			{
				SubCat.Indicators.Add(ParseIndicator(IndicatorValue->AsObject()));
			}
		}
	}

	return SubCat;
}

FIndicatorCategory FIndicatorDataLoader::ParseCategory(const TSharedPtr<FJsonObject>& JsonObj)
{
	FIndicatorCategory Category;
	JsonObj->TryGetStringField(TEXT("id"), Category.Id);
	JsonObj->TryGetStringField(TEXT("name"), Category.Name);

	const TArray<TSharedPtr<FJsonValue>>* SubCategoriesArray;
	if (JsonObj->TryGetArrayField(TEXT("subCategories"), SubCategoriesArray))
	{
		Category.SubCategories.Empty();
		for (const TSharedPtr<FJsonValue>& SubCatValue : *SubCategoriesArray)
		{
			if (SubCatValue->Type == EJson::Object)
			{
				Category.SubCategories.Add(ParseSubCategory(SubCatValue->AsObject()));
			}
		}
	}

	return Category;
}
